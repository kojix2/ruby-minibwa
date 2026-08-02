#include "minibwa.h"

VALUE rb_mMinibwa;
VALUE rb_cMinibwaOptions;
VALUE rb_cMinibwaIndex;
VALUE rb_cMinibwaBuffer;
VALUE rb_cMinibwaHit;
VALUE rb_eMinibwaError;

/*
 * Document-module: Minibwa
 *
 * Ruby bindings for minibwa, a short-read aligner.
 */

/*
 * Document-class: Minibwa::Error
 *
 * Raised when minibwa cannot build, load, or use an index.
 */

/* ------------------------------------------------------------------ */
/* TypedData unwrapping helpers                                       */
/* ------------------------------------------------------------------ */

mb_opt_t *
rb_minibwa_get_opt(VALUE self)
{
    mb_opt_t *opt;
    TypedData_Get_Struct(self, mb_opt_t, &rb_minibwa_options_type, opt);
    return opt;
}

mb_idx_t *
rb_minibwa_get_idx(VALUE self)
{
    mb_idx_t *idx;
    TypedData_Get_Struct(self, mb_idx_t, &rb_minibwa_idx_type, idx);
    return idx;
}

mb_tbuf_t *
rb_minibwa_get_tbuf(VALUE self)
{
    if (NIL_P(self)) return NULL;
    mb_tbuf_t *b;
    TypedData_Get_Struct(self, mb_tbuf_t, &rb_minibwa_buffer_type, b);
    return b;
}

/* ------------------------------------------------------------------ */
/* Init                                                               */
/* ------------------------------------------------------------------ */

RUBY_FUNC_EXPORTED void
Init_minibwa(void)
{
    rb_mMinibwa = rb_define_module("Minibwa");

    rb_eMinibwaError = rb_define_class_under(rb_mMinibwa, "Error", rb_eStandardError);

    rb_minibwa_init_options();
    rb_minibwa_init_index();
    rb_minibwa_init_buffer();
    rb_minibwa_init_hit();
    rb_minibwa_init_index_build();
}
