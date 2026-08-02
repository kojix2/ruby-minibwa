/*
 * Minibwa::Index.build -- FM-index construction.
 *
 * This is the one part of minibwa with no public API: upstream exposes index
 * construction only through main_index(argc, argv) in index.c, and its worker
 * mb_bwt_libsais() is static. Rather than synthesise an argv and inherit the
 * CLI's usage messages and exit paths, this file #includes index.c to reach
 * that static function and reimplements only the libsais branch
 * (l2b_import -> l2b_save -> mb_bwt_libsais -> mb_bwt_save). extconf.rb must
 * therefore keep index.c out of the source list, or every symbol in it would
 * be defined twice.
 *
 * The low-memory branch is intentionally not offered: it needs bwtgen.c and
 * QSufSort.c, which are GPL'd and are not shipped with this MIT gem.
 *
 * Construction runs with the GVL released. Note that upstream signals bad
 * input with kom_assert(), which abort()s the process, so inputs are
 * validated on the Ruby side before reaching here.
 */

#include "minibwa.h"
#include <stdlib.h>
#include <string.h>

/*
 * Include index.c to reach the static mb_bwt_libsais().  We must suppress
 * main() and the other entry points.  index.c has no include guard, so we
 * rely on extconf.rb keeping it out of $srcs.
 *
 * The static functions we need:
 *   l2b_c2t, l2b_g2a, sa_to_bwt, mb_bwt_libsais
 * The public functions we call:
 *   l2b_import, l2b_save, mb_bwt_save, mb_bwt_init_from_raw
 */
#include "minibwa/index.c"

static char *
rb_minibwa_prefixed_path(const char *prefix, const char *suffix)
{
    size_t prefix_len = strlen(prefix);
    size_t suffix_len = strlen(suffix);
    char *path = malloc(prefix_len + suffix_len + 1);
    if (!path) return NULL;

    memcpy(path, prefix, prefix_len);
    memcpy(path + prefix_len, suffix, suffix_len + 1);
    return path;
}

/* ------------------------------------------------------------------ */
/* Build body (runs without GVL)                                      */
/* ------------------------------------------------------------------ */

struct build_args {
    const char *fn_fa;
    const char *path_l2b;
    const char *path_bwt;
    int sa_bit;
    int n_thread;
    uint64_t seed;
    int is_meth;
    int error;
};

static void *
rb_minibwa_index_build_body(void *ptr)
{
    struct build_args *args = (struct build_args *)ptr;

    args->error = 0;

    /* Step 1: FASTA → l2b (2-bit encoding) */
    l2b_t *l2b = l2b_import(args->fn_fa, args->seed);
    if (!l2b) { args->error = 1; return NULL; }

    l2b_save(args->path_l2b, l2b);

    /* Step 2: l2b → BWT via libsais */
    mb_bwt_t *bwt = mb_bwt_libsais(l2b, args->sa_bit, 1, args->is_meth, args->n_thread);
    l2b_destroy(l2b);

    if (!bwt) { args->error = 2; return NULL; }

    /* Upstream names the BWT file "<prefix>.mbw" (see main_index in
     * index.c); mb_idx_load() looks for exactly that. */
    mb_bwt_save(args->path_bwt, bwt);
    mb_bwt_destroy(bwt);

    return NULL;
}

/* ------------------------------------------------------------------ */
/* Ruby method                                                        */
/* ------------------------------------------------------------------ */

/*
 * call-seq:
 *   Index.build(fasta, prefix, sa_bit: 4, n_thread: 4, seed: 11, meth: false) -> true
 *
 * Builds a minibwa index from a FASTA file.
 *
 * +fasta+::    path to the input FASTA file.
 * +prefix+::   output path prefix (writes +prefix+.l2b and +prefix+.mbw).
 * +sa_bit+::   suffix array sample rate (1/(1<<sa_bit)).
 * +n_thread+:: number of threads for libsais (requires OpenMP).
 * +seed+::     random seed for hash table in l2b_import.
 * +meth+::     build methylation index (forward C→T and G→A strands).
 *
 * Returns +true+ on success.  Raises Minibwa::Error on failure.
 *
 * NOTE: upstream signals bad input with kom_assert(), which abort()s the
 * process.  Validate inputs on the Ruby side before calling this method.
 */
static VALUE
rb_minibwa_index_build(int argc, VALUE *argv, VALUE klass)
{
    VALUE fasta_val, prefix_val, kwargs;
    rb_scan_args(argc, argv, "2:", &fasta_val, &prefix_val, &kwargs);

    const char *fn_fa  = StringValueCStr(fasta_val);
    const char *prefix = StringValueCStr(prefix_val);

    int sa_bit   = 4;
    int n_thread = 4;
    uint64_t seed = 11;
    int is_meth  = 0;

    if (!NIL_P(kwargs)) {
        VALUE v;
        v = rb_hash_aref(kwargs, ID2SYM(rb_intern("sa_bit")));
        if (!NIL_P(v)) sa_bit = NUM2INT(v);
        v = rb_hash_aref(kwargs, ID2SYM(rb_intern("n_thread")));
        if (!NIL_P(v)) n_thread = NUM2INT(v);
        v = rb_hash_aref(kwargs, ID2SYM(rb_intern("seed")));
        if (!NIL_P(v)) seed = NUM2ULL(v);
        v = rb_hash_aref(kwargs, ID2SYM(rb_intern("meth")));
        if (RTEST(v)) is_meth = 1;
    }

    char *path_l2b = rb_minibwa_prefixed_path(prefix, ".l2b");
    char *path_bwt = rb_minibwa_prefixed_path(prefix, ".mbw");
    if (!path_l2b || !path_bwt) {
        free(path_l2b);
        free(path_bwt);
        rb_memerror();
    }

    struct build_args args = {
        fn_fa, path_l2b, path_bwt, sa_bit, n_thread, seed, is_meth, 0
    };
    rb_thread_call_without_gvl(rb_minibwa_index_build_body, &args, NULL, NULL);

    free(path_l2b);
    free(path_bwt);

    if (args.error)
        rb_raise(rb_eMinibwaError, "index build failed for \"%s\" (error %d)", fn_fa, args.error);

    return Qtrue;
}

/* ------------------------------------------------------------------ */
/* Registration                                                       */
/* ------------------------------------------------------------------ */

void
rb_minibwa_init_index_build(void)
{
    /* Index.build is a singleton method on Minibwa::Index */
    rb_define_singleton_method(rb_cMinibwaIndex, "build",
                               rb_minibwa_index_build, -1);
}
