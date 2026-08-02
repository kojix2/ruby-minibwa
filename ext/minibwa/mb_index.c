/*
 * Minibwa::Index -- wraps mb_idx_t, the loaded FM-index and 2-bit reference,
 * and is where mapping is driven from.
 *
 * Opening (mb_idx_load, mb_idx_load_mmap) and mapping (mb_map, mb_map_batch)
 * are long-running and CPU-bound, so all of them release the GVL. Query
 * sequences and names are kept alive via Ruby references for the duration
 * of the call; callers must not mutate them while the call is in flight.
 *
 * mb_idx_load() reports a missing or corrupt index by returning NULL, which
 * becomes Minibwa::Error. Contig lookups are range-checked upstream (NULL or
 * -1 for an out-of-range tid) and become IndexError.
 *
 * Freed with mb_idx_destroy().
 */

#include "minibwa.h"

static VALUE
rb_minibwa_kw(VALUE kwargs, const char *name)
{
    if (NIL_P(kwargs)) return Qnil;
    return rb_hash_aref(kwargs, ID2SYM(rb_intern(name)));
}

static int
rb_minibwa_kw_bool(VALUE kwargs, const char *name)
{
    return RTEST(rb_minibwa_kw(kwargs, name));
}

static mb_opt_t *
rb_minibwa_kw_opt(VALUE kwargs, mb_opt_t *default_opt)
{
    VALUE opt_val = rb_minibwa_kw(kwargs, "opt");
    if (!NIL_P(opt_val)) return rb_minibwa_get_opt(opt_val);

    mb_opt_init(default_opt);
    return default_opt;
}

static mb_tbuf_t *
rb_minibwa_kw_tbuf(VALUE kwargs)
{
    return rb_minibwa_get_tbuf(rb_minibwa_kw(kwargs, "buf"));
}

static void
rb_minibwa_idx_free(void *ptr)
{
    mb_idx_t *idx = (mb_idx_t *)ptr;
    if (idx) mb_idx_destroy(idx);
}

static size_t
rb_minibwa_idx_memsize(const void *ptr)
{
    /* mb_idx_t is an opaque type; report the pointer size only. */
    (void)ptr;
    return 0;
}

const rb_data_type_t rb_minibwa_idx_type = {
    .wrap_struct_name = "Minibwa::Index",
    .function = {
        NULL,
        rb_minibwa_idx_free,
        rb_minibwa_idx_memsize,
    },
    .flags = RUBY_TYPED_FREE_IMMEDIATELY
};

/* ------------------------------------------------------------------ */
/* Construction: .load, .load_mmap                                    */
/* ------------------------------------------------------------------ */

struct idx_load_args {
    const char *prefix;
    int32_t is_meth;
    int preload;       /* only for mmap variant */
    int mmap;          /* 0 = load, 1 = load_mmap */
    mb_idx_t *result;
};

static void *
rb_minibwa_idx_load_body(void *ptr)
{
    struct idx_load_args *args = (struct idx_load_args *)ptr;
    if (args->mmap)
        args->result = mb_idx_load_mmap(args->prefix, args->is_meth, args->preload);
    else
        args->result = mb_idx_load(args->prefix, args->is_meth);
    return NULL;
}

/*
 * call-seq:
 *   Index.load(prefix, meth: false) -> Index
 *
 * Loads an index from disk.  +prefix+ is the path prefix of the index files
 * (e.g. <tt>"ref"</tt> loads <tt>ref.l2b</tt> and <tt>ref.mbw</tt>).
 * Set +meth:+ to +true+ for methylation mode.
 *
 * Raises Minibwa::Error if the index cannot be loaded.
 */
static VALUE
rb_minibwa_idx_load(int argc, VALUE *argv, VALUE klass)
{
    VALUE prefix_val, kwargs;
    rb_scan_args(argc, argv, "1:", &prefix_val, &kwargs);

    const char *prefix = StringValueCStr(prefix_val);

    struct idx_load_args args = { prefix, rb_minibwa_kw_bool(kwargs, "meth"), 0, 0, NULL };
    rb_thread_call_without_gvl(rb_minibwa_idx_load_body, &args, NULL, NULL);

    if (!args.result)
        rb_raise(rb_eMinibwaError, "failed to load index from \"%s\"", prefix);

    VALUE obj = TypedData_Wrap_Struct(klass, &rb_minibwa_idx_type, args.result);
    return obj;
}

/*
 * call-seq:
 *   Index.load_mmap(prefix, meth: false, preload: false) -> Index
 *
 * Loads an index using memory-mapped I/O.  Set +preload:+ to +true+ to
 * preload the index into memory.
 */
static VALUE
rb_minibwa_idx_load_mmap(int argc, VALUE *argv, VALUE klass)
{
    VALUE prefix_val, kwargs;
    rb_scan_args(argc, argv, "1:", &prefix_val, &kwargs);

    const char *prefix = StringValueCStr(prefix_val);

    struct idx_load_args args = {
        prefix, rb_minibwa_kw_bool(kwargs, "meth"),
        rb_minibwa_kw_bool(kwargs, "preload"), 1, NULL
    };
    rb_thread_call_without_gvl(rb_minibwa_idx_load_body, &args, NULL, NULL);

    if (!args.result)
        rb_raise(rb_eMinibwaError, "failed to load index from \"%s\"", prefix);

    VALUE obj = TypedData_Wrap_Struct(klass, &rb_minibwa_idx_type, args.result);
    return obj;
}

/* ------------------------------------------------------------------ */
/* Contig queries                                                     */
/* ------------------------------------------------------------------ */

/*
 * call-seq:
 *   ctg_name(tid) -> String or nil
 *
 * Returns the contig name for the given target ID, or +nil+ if +tid+ is out
 * of range.
 */
static VALUE
rb_minibwa_idx_ctg_name(VALUE self, VALUE tid_val)
{
    mb_idx_t *idx = rb_minibwa_get_idx(self);
    int32_t tid = NUM2INT(tid_val);
    const char *name = mb_idx_ctg_name(idx, tid);
    if (!name) return Qnil;
    return rb_str_new_cstr(name);
}

/*
 * call-seq:
 *   ctg_len(tid) -> Integer or nil
 *
 * Returns the contig length for the given target ID, or +nil+ if +tid+ is
 * out of range.
 */
static VALUE
rb_minibwa_idx_ctg_len(VALUE self, VALUE tid_val)
{
    mb_idx_t *idx = rb_minibwa_get_idx(self);
    int32_t tid = NUM2INT(tid_val);
    int64_t len = mb_idx_ctg_len(idx, tid);
    if (len < 0) return Qnil;
    return LL2NUM(len);
}

/* ------------------------------------------------------------------ */
/* Mapping: single sequence                                           */
/* ------------------------------------------------------------------ */

struct map_args {
    const mb_opt_t *opt;
    const mb_idx_t *idx;
    int32_t qlen;
    const char *seq;
    int32_t mt;
    mb_tbuf_t *b;
    const char *qname;
    mb_hit_t *result;
    int32_t n_hit;
};

static void *
rb_minibwa_map_body(void *ptr)
{
    struct map_args *args = (struct map_args *)ptr;
    args->result = mb_map(args->opt, args->idx, args->qlen, args->seq,
                          args->mt, &args->n_hit, args->b, args->qname);
    return NULL;
}

/*
 * call-seq:
 *   map(seq, name: nil, opt: nil, buf: nil, meth: 0) -> Array of Hit
 *
 * Aligns one query sequence against the index.
 *
 * +seq+::   query sequence as a String (ASCII bases).
 * +name+::  optional query name.
 * +opt+::   an Options object; uses defaults if omitted.
 * +buf+::   a Buffer for scratch space; allocates internally if +nil+.
 * +meth+::  methylation type: 0 (none), 1 (read1 C-to-T), 2 (read2 G-to-A).
 *
 * Returns an Array of Minibwa::Hit objects.
 */
static VALUE
rb_minibwa_idx_map(int argc, VALUE *argv, VALUE self)
{
    VALUE seq_val, kwargs;
    rb_scan_args(argc, argv, "1:", &seq_val, &kwargs);

    mb_idx_t *idx = rb_minibwa_get_idx(self);

    /* --- options --- */
    mb_opt_t default_opt;
    mb_opt_t *opt = rb_minibwa_kw_opt(kwargs, &default_opt);
    mb_tbuf_t *b = rb_minibwa_kw_tbuf(kwargs);

    /* --- methylation --- */
    int32_t mt = 0;
    VALUE mt_val = rb_minibwa_kw(kwargs, "meth");
    if (!NIL_P(mt_val)) mt = NUM2INT(mt_val);

    /* --- query name --- */
    const char *qname = NULL;
    VALUE name_val = rb_minibwa_kw(kwargs, "name");
    if (!NIL_P(name_val)) qname = StringValueCStr(name_val);

    /* --- sequence --- */
    const char *seq = StringValueCStr(seq_val);
    int32_t qlen = (int32_t)RSTRING_LEN(seq_val);

    struct map_args args = { opt, idx, qlen, seq, mt, b, qname, NULL, 0 };
    rb_thread_call_without_gvl(rb_minibwa_map_body, &args, NULL, NULL);

    return rb_minibwa_hit_ary_new(idx, args.result, args.n_hit);
}

/* ------------------------------------------------------------------ */
/* Mapping: batch                                                      */
/* ------------------------------------------------------------------ */

struct batch_args {
    const mb_opt_t *opt;
    const mb_idx_t *idx;
    int32_t n_seq;
    const int32_t *qlen;
    const char **seq;
    mb_tbuf_t *b;
    const char **qname;
    mb_hit_t **result;
    int32_t *n_hit;
};

static void *
rb_minibwa_map_batch_body(void *ptr)
{
    struct batch_args *args = (struct batch_args *)ptr;
    args->result = mb_map_batch(args->opt, args->idx, args->n_seq,
                                args->qlen, args->seq, args->n_hit,
                                args->b, args->qname);
    return NULL;
}

/*
 * call-seq:
 *   map_batch(seqs, names: nil, opt: nil, buf: nil) -> Array of Array of Hit
 *
 * Aligns multiple query sequences in one batch.
 *
 * +seqs+::  an Array of query sequence Strings.
 * +names+:: optional Array of query names (same length as +seqs+).
 * +opt+::   an Options object; uses defaults if omitted.
 * +buf+::   a Buffer for scratch space; allocates internally if +nil+.
 *
 * Returns an Array of Arrays of Minibwa::Hit, one per input sequence.
 */

struct map_batch_ctx {
    VALUE seqs_val;
    VALUE names_val;
    int32_t n_seq;
    const mb_opt_t *opt;
    mb_idx_t *idx;
    mb_tbuf_t *b;
    int32_t *qlen;
    char **seq;
    char **qname;
    int32_t *n_hit;
    mb_hit_t **result;   /* set by mb_map_batch; NULL if not yet called */
    VALUE pins;
};

static VALUE
rb_minibwa_map_batch_work(VALUE ptr)
{
    struct map_batch_ctx *ctx = (struct map_batch_ctx *)ptr;

    /* --- build C arrays --- */
    ctx->qlen  = ALLOC_N(int32_t, ctx->n_seq);
    ctx->seq   = ALLOC_N(char *,   ctx->n_seq);
    if (!NIL_P(ctx->names_val))
        ctx->qname = ALLOC_N(char *, ctx->n_seq);
    ctx->n_hit = ALLOC_N(int32_t, ctx->n_seq);

    /* Keep converted Strings alive across the GVL-free call.  A Ruby
     * array on the C stack is pinned by conservative scanning. */
    ctx->pins = rb_ary_new_capa(ctx->n_seq);

    for (int32_t i = 0; i < ctx->n_seq; i++) {
        VALUE s = rb_ary_entry(ctx->seqs_val, i);
        StringValue(s);
        rb_ary_push(ctx->pins, s);
        ctx->qlen[i]  = (int32_t)RSTRING_LEN(s);
        ctx->seq[i]   = (char *)StringValueCStr(s); /* NUL-terminated, safe */
        if (ctx->qname) {
            VALUE nm = rb_ary_entry(ctx->names_val, i);
            if (!NIL_P(nm)) {
                StringValue(nm);
                rb_ary_push(ctx->pins, nm);
                ctx->qname[i] = (char *)StringValueCStr(nm);
            } else {
                ctx->qname[i] = NULL;
            }
        }
    }

    struct batch_args args = { ctx->opt, ctx->idx, ctx->n_seq, ctx->qlen,
                               (const char **)ctx->seq, ctx->b,
                               (const char **)ctx->qname, NULL, ctx->n_hit };
    rb_thread_call_without_gvl(rb_minibwa_map_batch_body, &args, NULL, NULL);

    /* mb_map_batch() returns NULL when it cannot run at all (e.g. meth
     * mode requested on a non-meth index).  Detect this before touching
     * args.result[] or n_hit[]. */
    if (!args.result) {
        rb_raise(rb_eMinibwaError,
                 "mb_map_batch failed (methylation mode mismatch?)");
    }

    ctx->result = args.result;

    /* --- convert results --- */
    VALUE ary = rb_ary_new_capa(ctx->n_seq);
    for (int32_t i = 0; i < ctx->n_seq; i++) {
        VALUE hits = rb_minibwa_hit_ary_new(ctx->idx, ctx->result[i], ctx->n_hit[i]);
        rb_ary_push(ary, hits);
    }

    /* args.result comes from upstream's calloc → free(). */
    free(ctx->result);
    ctx->result = NULL;

    return ary;
}

static VALUE
rb_minibwa_map_batch_cleanup(VALUE ptr)
{
    struct map_batch_ctx *ctx = (struct map_batch_ctx *)ptr;
    /* These are ruby_xmalloc → xfree().  Safe to call even on the
     * success path (xfree(NULL) is a no-op). */
    xfree(ctx->qlen);
    xfree(ctx->seq);
    xfree(ctx->qname);
    xfree(ctx->n_hit);
    /* If the work function raised after mb_map_batch() succeeded but
     * before all hits were converted, free the result array. */
    free(ctx->result);
    return Qnil;
}

static VALUE
rb_minibwa_idx_map_batch(int argc, VALUE *argv, VALUE self)
{
    VALUE seqs_val, kwargs;
    rb_scan_args(argc, argv, "1:", &seqs_val, &kwargs);

    mb_idx_t *idx = rb_minibwa_get_idx(self);
    Check_Type(seqs_val, T_ARRAY);

    int32_t n_seq = (int32_t)RARRAY_LEN(seqs_val);
    if (n_seq == 0) return rb_ary_new();

    /* --- options --- */
    mb_opt_t default_opt;
    mb_opt_t *opt = rb_minibwa_kw_opt(kwargs, &default_opt);
    mb_tbuf_t *b = rb_minibwa_kw_tbuf(kwargs);

    /* --- names --- */
    VALUE names_val = rb_minibwa_kw(kwargs, "names");
    if (!NIL_P(names_val)) {
        Check_Type(names_val, T_ARRAY);
        if (RARRAY_LEN(names_val) != n_seq)
            rb_raise(rb_eArgError, "names length (%ld) must match seqs length (%d)",
                     RARRAY_LEN(names_val), n_seq);
    }

    struct map_batch_ctx ctx = {
        .seqs_val = seqs_val,
        .names_val = names_val,
        .n_seq = n_seq,
        .opt = opt,
        .idx = idx,
        .b = b,
        .qlen = NULL,
        .seq = NULL,
        .qname = NULL,
        .n_hit = NULL,
        .result = NULL,
        .pins = Qnil
    };

    return rb_ensure(rb_minibwa_map_batch_work, (VALUE)&ctx,
                     rb_minibwa_map_batch_cleanup, (VALUE)&ctx);
}

/* ------------------------------------------------------------------ */
/* Registration                                                       */
/* ------------------------------------------------------------------ */

void
rb_minibwa_init_index(void)
{
    VALUE klass = rb_define_class_under(rb_mMinibwa, "Index", rb_cObject);
    rb_cMinibwaIndex = klass;

    /* Index is only ever created via .load / .load_mmap (which use
     * TypedData_Wrap_Struct directly), so Index.new is not supported. */
    rb_undef_alloc_func(klass);

    rb_define_singleton_method(klass, "load",      rb_minibwa_idx_load,      -1);
    rb_define_singleton_method(klass, "load_mmap", rb_minibwa_idx_load_mmap, -1);

    rb_define_method(klass, "ctg_name",  rb_minibwa_idx_ctg_name,  1);
    rb_define_method(klass, "ctg_len",   rb_minibwa_idx_ctg_len,   1);
    rb_define_method(klass, "map",       rb_minibwa_idx_map,       -1);
    rb_define_method(klass, "map_batch", rb_minibwa_idx_map_batch, -1);
}
