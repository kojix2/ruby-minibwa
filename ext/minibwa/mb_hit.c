/*
 * Minibwa::Hit -- one alignment produced by mb_map() / mb_map_batch().
 *
 * mb_hit_t is converted eagerly into a plain Ruby object and the C memory is
 * released immediately, because the caller owns both hit[i].p and the hit
 * array. Copying up front costs one pass but removes every question about
 * object lifetime, and keeps a Hit usable after its Index is gone.
 *
 * The CIGAR in mb_extra_t is unpacked into an array of [length, op] pairs.
 * It describes the aligned region only and contains no soft or hard clipping;
 * clips have to be reconstructed from qs/qe (see lib/minibwa/hit.rb).
 */

#include "minibwa.h"

/* ------------------------------------------------------------------ */
/* Single hit conversion                                              */
/* ------------------------------------------------------------------ */

/*
 * Builds a Minibwa::Hit from an mb_hit_t.  Takes ownership of hit->p
 * (frees it).  The idx is needed for ctg_name lookups.
 */
VALUE
rb_minibwa_hit_new(const mb_idx_t *idx, mb_hit_t *hit)
{
    VALUE klass = rb_cMinibwaHit;
    VALUE obj = rb_obj_alloc(klass);

    /* --- scalar fields --- */
    rb_ivar_set(obj, rb_intern("@tid"),    LL2NUM(hit->tid));
    rb_ivar_set(obj, rb_intern("@ts"),     LL2NUM(hit->ts));
    rb_ivar_set(obj, rb_intern("@te"),     LL2NUM(hit->te));
    rb_ivar_set(obj, rb_intern("@qs"),     INT2NUM(hit->qs));
    rb_ivar_set(obj, rb_intern("@qe"),     INT2NUM(hit->qe));
    rb_ivar_set(obj, rb_intern("@score"),  INT2NUM(hit->score));
    rb_ivar_set(obj, rb_intern("@score0"), INT2NUM(hit->score0));
    rb_ivar_set(obj, rb_intern("@mlen"),   INT2NUM(hit->mlen));
    rb_ivar_set(obj, rb_intern("@blen"),   INT2NUM(hit->blen));
    rb_ivar_set(obj, rb_intern("@mapq"),   INT2NUM(hit->mapq));
    rb_ivar_set(obj, rb_intern("@cnt"),    INT2NUM(hit->cnt));
    rb_ivar_set(obj, rb_intern("@n_sub"),  INT2NUM(hit->n_sub));
    rb_ivar_set(obj, rb_intern("@subsc"),  INT2NUM(hit->subsc));
    rb_ivar_set(obj, rb_intern("@hash"),   UINT2NUM(hit->hash));

    /* --- bit fields --- */
    rb_ivar_set(obj, rb_intern("@rev"),         hit->rev         ? Qtrue : Qfalse);
    rb_ivar_set(obj, rb_intern("@proper_pair"), hit->proper_pair ? Qtrue : Qfalse);
    rb_ivar_set(obj, rb_intern("@sam_pri"),     hit->sam_pri     ? Qtrue : Qfalse);
    rb_ivar_set(obj, rb_intern("@flt"),         hit->flt         ? Qtrue : Qfalse);
    rb_ivar_set(obj, rb_intern("@inv"),         hit->inv         ? Qtrue : Qfalse);
    rb_ivar_set(obj, rb_intern("@split"),       INT2NUM(hit->split));
    rb_ivar_set(obj, rb_intern("@split_inv"),   hit->split_inv   ? Qtrue : Qfalse);
    rb_ivar_set(obj, rb_intern("@rescued"),     hit->rescued     ? Qtrue : Qfalse);
    rb_ivar_set(obj, rb_intern("@frac_high"),   INT2NUM(hit->frac_high));
    rb_ivar_set(obj, rb_intern("@seed_ratio"),  INT2NUM(hit->seed_ratio));

    /* --- contig name (lazy: store tid, resolve in Ruby) --- */
    const char *ctg = mb_idx_ctg_name(idx, (int32_t)hit->tid);
    rb_ivar_set(obj, rb_intern("@ctg"),
                ctg ? rb_str_new_cstr(ctg) : Qnil);

    /* --- extra (CIGAR, DP scores) --- */
    if (hit->p) {
        mb_extra_t *p = hit->p;

        rb_ivar_set(obj, rb_intern("@dp_score"), INT2NUM(p->dp_score));
        rb_ivar_set(obj, rb_intern("@dp_max0"),  INT2NUM(p->dp_max0));
        rb_ivar_set(obj, rb_intern("@dp_max"),   INT2NUM(p->dp_max));
        rb_ivar_set(obj, rb_intern("@dp_max2"),  INT2NUM(p->dp_max2));
        rb_ivar_set(obj, rb_intern("@n_ambi"),   UINT2NUM(p->n_ambi));
        rb_ivar_set(obj, rb_intern("@cs_flag"),  p->cs ? Qtrue : Qfalse);

        /* CIGAR: array of [length, op] pairs */
        VALUE cigar = rb_ary_new_capa(p->n_cigar);
        for (int32_t i = 0; i < p->n_cigar; i++) {
            uint32_t c = p->cigar[i];
            VALUE pair = rb_ary_new_from_args(2,
                UINT2NUM(c >> 4),
                INT2NUM(c & 0xf));
            rb_ary_push(cigar, pair);
        }
        rb_ivar_set(obj, rb_intern("@cigar"), cigar);

        free(p);
        hit->p = NULL;  /* mark as consumed for the ensure handler */
    } else {
        rb_ivar_set(obj, rb_intern("@dp_score"), INT2NUM(0));
        rb_ivar_set(obj, rb_intern("@dp_max0"),  INT2NUM(0));
        rb_ivar_set(obj, rb_intern("@dp_max"),   INT2NUM(0));
        rb_ivar_set(obj, rb_intern("@dp_max2"),  INT2NUM(0));
        rb_ivar_set(obj, rb_intern("@n_ambi"),   INT2NUM(0));
        rb_ivar_set(obj, rb_intern("@cs_flag"),  Qfalse);
        rb_ivar_set(obj, rb_intern("@cigar"),    rb_ary_new());
    }

    return obj;
}

/* ------------------------------------------------------------------ */
/* Hit array conversion                                               */
/* ------------------------------------------------------------------ */

struct hit_ary_ctx {
    const mb_idx_t *idx;
    mb_hit_t *hits;
    int32_t n_hit;
    VALUE ary;
};

static VALUE
rb_minibwa_hit_ary_convert(VALUE ptr)
{
    struct hit_ary_ctx *ctx = (struct hit_ary_ctx *)ptr;
    ctx->ary = rb_ary_new_capa(ctx->n_hit);
    for (int32_t i = 0; i < ctx->n_hit; i++) {
        VALUE hit = rb_minibwa_hit_new(ctx->idx, &ctx->hits[i]);
        rb_ary_push(ctx->ary, hit);
    }
    return ctx->ary;
}

static VALUE
rb_minibwa_hit_ary_free_hits(VALUE ptr)
{
    struct hit_ary_ctx *ctx = (struct hit_ary_ctx *)ptr;
    /* If conversion raised, free any remaining hit[i].p and the array.
     * rb_minibwa_hit_new() sets hit->p to NULL after freeing, so already-
     * converted hits are skipped. */
    for (int32_t i = 0; i < ctx->n_hit; i++)
        free(ctx->hits[i].p);
    free(ctx->hits);
    return Qnil;
}

/*
 * Converts an array of mb_hit_t to a Ruby Array of Minibwa::Hit.
 * Takes ownership of the hit array and all hit[i].p (frees them).
 * If conversion raises, the array is still freed before re-raising.
 */
VALUE
rb_minibwa_hit_ary_new(const mb_idx_t *idx, mb_hit_t *hits, int32_t n_hit)
{
    struct hit_ary_ctx ctx = { idx, hits, n_hit, Qnil };
    return rb_ensure(rb_minibwa_hit_ary_convert, (VALUE)&ctx,
                     rb_minibwa_hit_ary_free_hits, (VALUE)&ctx);
}

/* ------------------------------------------------------------------ */
/* Registration                                                       */
/* ------------------------------------------------------------------ */

void
rb_minibwa_init_hit(void)
{
    VALUE klass = rb_define_class_under(rb_mMinibwa, "Hit", rb_cObject);
    rb_cMinibwaHit = klass;

    /* attr_readers for all fields */
    rb_define_attr(klass, "tid",         1, 0);
    rb_define_attr(klass, "ts",          1, 0);
    rb_define_attr(klass, "te",          1, 0);
    rb_define_attr(klass, "qs",          1, 0);
    rb_define_attr(klass, "qe",          1, 0);
    rb_define_attr(klass, "score",       1, 0);
    rb_define_attr(klass, "score0",      1, 0);
    rb_define_attr(klass, "mlen",        1, 0);
    rb_define_attr(klass, "blen",        1, 0);
    rb_define_attr(klass, "mapq",        1, 0);
    rb_define_attr(klass, "cnt",         1, 0);
    rb_define_attr(klass, "n_sub",       1, 0);
    rb_define_attr(klass, "subsc",       1, 0);
    rb_define_attr(klass, "hash",        1, 0);
    rb_define_attr(klass, "rev",         1, 0);
    rb_define_attr(klass, "proper_pair", 1, 0);
    rb_define_attr(klass, "sam_pri",     1, 0);
    rb_define_attr(klass, "flt",         1, 0);
    rb_define_attr(klass, "inv",         1, 0);
    rb_define_attr(klass, "split",       1, 0);
    rb_define_attr(klass, "split_inv",   1, 0);
    rb_define_attr(klass, "rescued",     1, 0);
    rb_define_attr(klass, "frac_high",   1, 0);
    rb_define_attr(klass, "seed_ratio",  1, 0);
    rb_define_attr(klass, "ctg",         1, 0);
    rb_define_attr(klass, "dp_score",    1, 0);
    rb_define_attr(klass, "dp_max0",     1, 0);
    rb_define_attr(klass, "dp_max",      1, 0);
    rb_define_attr(klass, "dp_max2",     1, 0);
    rb_define_attr(klass, "n_ambi",      1, 0);
    rb_define_attr(klass, "cs_flag",     1, 0);
    rb_define_attr(klass, "cigar",       1, 0);
}
