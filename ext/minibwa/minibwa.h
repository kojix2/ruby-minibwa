/*
 * Shared header for the Ruby binding: module/class handles, the per-file
 * initializers called from Init_minibwa(), and the helpers that cross
 * translation units.
 */

#ifndef RB_MINIBWA_H
/* NOT "MINIBWA_H": that is upstream's guard. Sharing it would make
 * whichever header is included second expand to nothing. */
#define RB_MINIBWA_H 1

#include "ruby.h"
#include "ruby/thread.h"

/* Upstream public API. Spelled with the directory prefix because this file
 * shares its basename with it, and a bare "minibwa.h" from extension sources
 * resolves to this file first. */
#include "minibwa/minibwa.h"

/* Module, classes and exception, defined in minibwa.c */
extern VALUE rb_mMinibwa;
extern VALUE rb_cMinibwaOptions;
extern VALUE rb_cMinibwaIndex;
extern VALUE rb_cMinibwaBuffer;
extern VALUE rb_cMinibwaHit;
extern VALUE rb_eMinibwaError;

/* Per-file initializers, called in this order from Init_minibwa() */
void rb_minibwa_init_options(void);
void rb_minibwa_init_index(void);
void rb_minibwa_init_buffer(void);
void rb_minibwa_init_hit(void);
void rb_minibwa_init_index_build(void);

/* TypedData unwrapping. Each raises TypeError on a mismatched receiver;
 * rb_minibwa_get_tbuf() maps Qnil to NULL, which mb_map() accepts. */
mb_opt_t *rb_minibwa_get_opt(VALUE self);
mb_idx_t *rb_minibwa_get_idx(VALUE self);
mb_tbuf_t *rb_minibwa_get_tbuf(VALUE self);

/* TypedData type descriptors, defined in their respective files */
extern const rb_data_type_t rb_minibwa_options_type;
extern const rb_data_type_t rb_minibwa_idx_type;
extern const rb_data_type_t rb_minibwa_buffer_type;

/* mb_hit.c: conversion to Ruby. Both take ownership -- they free hit->p
 * for every hit, and rb_minibwa_hit_ary_new() also frees the array. */
VALUE rb_minibwa_hit_new(const mb_idx_t *idx, mb_hit_t *hit);
VALUE rb_minibwa_hit_ary_new(const mb_idx_t *idx, mb_hit_t *hits, int32_t n_hit);

#endif /* RB_MINIBWA_H */
