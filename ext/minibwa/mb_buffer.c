/*
 * Document-class: Minibwa::Buffer
 *
 * Minibwa::Buffer -- wraps mb_tbuf_t, the reusable per-thread scratch buffer.
 *
 * Passing a Buffer to Index#map saves an mb_tbuf_init/mb_tbuf_destroy pair on
 * every call; passing nil lets mb_map() allocate one internally. A Buffer
 * carries mutable working state and must never be shared between threads that
 * map concurrently -- one Buffer per thread.
 *
 * Freed with mb_tbuf_destroy().
 */

#include "minibwa.h"

static void
rb_minibwa_buffer_free(void *ptr)
{
    mb_tbuf_t *b = (mb_tbuf_t *)ptr;
    if (b) mb_tbuf_destroy(b);
}

static size_t
rb_minibwa_buffer_memsize(const void *ptr)
{
    /* mb_tbuf_t is an opaque type; report the pointer size only. */
    (void)ptr;
    return 0;
}

const rb_data_type_t rb_minibwa_buffer_type = {
    .wrap_struct_name = "Minibwa::Buffer",
    .function = {
        NULL,
        rb_minibwa_buffer_free,
        rb_minibwa_buffer_memsize,
    },
    .flags = RUBY_TYPED_FREE_IMMEDIATELY
};

/* ------------------------------------------------------------------ */
/* Constructor                                                        */
/* ------------------------------------------------------------------ */

/*
 * call-seq:
 *   Buffer.new(no_kalloc: false) -> Buffer
 *
 * Allocates a new thread-local scratch buffer.  Set +no_kalloc:+ to +true+
 * to disable the internal allocator.
 */
static VALUE
rb_minibwa_buffer_initialize(int argc, VALUE *argv, VALUE self)
{
    VALUE kwargs;
    rb_scan_args(argc, argv, ":", &kwargs);

    int no_kalloc = 0;
    if (!NIL_P(kwargs)) {
        VALUE v = rb_hash_aref(kwargs, ID2SYM(rb_intern("no_kalloc")));
        if (RTEST(v)) no_kalloc = 1;
    }

    /* The buffer was allocated by alloc with the default (kalloc enabled).
     * If the caller wants no_kalloc, rebuild it through the public API. */
    if (no_kalloc) {
        mb_tbuf_t *b = rb_minibwa_get_tbuf(self);
        if (b) mb_tbuf_destroy(b);
        b = mb_tbuf_init(1);
        if (!b) rb_raise(rb_eMinibwaError, "mb_tbuf_init failed");
        DATA_PTR(self) = b;
    }
    return Qnil;
}

static VALUE
rb_minibwa_buffer_alloc(VALUE klass)
{
    mb_tbuf_t *b = mb_tbuf_init(0);
    VALUE obj = TypedData_Wrap_Struct(klass, &rb_minibwa_buffer_type, b);
    return obj;
}

/*
 * call-seq:
 *   reset!(max_block_size) -> Integer
 *
 * Resets the buffer for reuse, optionally setting a new maximum block size.
 * Returns the new capacity.
 */
static VALUE
rb_minibwa_buffer_reset(VALUE self, VALUE max_block_size)
{
    mb_tbuf_t *b = rb_minibwa_get_tbuf(self);
    int64_t mbs = NUM2LL(max_block_size);
    int32_t cap = mb_tbuf_reset(b, mbs);
    return INT2NUM(cap);
}

/* ------------------------------------------------------------------ */
/* Registration                                                       */
/* ------------------------------------------------------------------ */

void
rb_minibwa_init_buffer(void)
{
    VALUE klass = rb_define_class_under(rb_mMinibwa, "Buffer", rb_cObject);
    rb_cMinibwaBuffer = klass;

    rb_define_alloc_func(klass, rb_minibwa_buffer_alloc);
    rb_define_method(klass, "initialize", rb_minibwa_buffer_initialize, -1);
    rb_define_method(klass, "reset!", rb_minibwa_buffer_reset, 1);
}
