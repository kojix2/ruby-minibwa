/*
 * Minibwa::Options -- wraps mb_opt_t, the alignment parameter block.
 *
 * The struct is stored by value inside a TypedData object, so copying an
 * Options never aliases C memory. Every one of its ~50 fields is exposed as
 * a reader/writer pair generated from an X-macro table, keeping the accessor
 * list to one line per field as upstream adds them. Also wraps mb_opt_init()
 * and mb_opt_preset() ("sr", "adap", "lr") and turns each MB_F_* bit into a
 * predicate/setter pair.
 */

#include "minibwa.h"

const rb_data_type_t rb_minibwa_options_type = {
    .wrap_struct_name = "Minibwa::Options",
    .function = { NULL, RUBY_DEFAULT_FREE, NULL },
    .flags = RUBY_TYPED_FREE_IMMEDIATELY
};

/* ------------------------------------------------------------------ */
/* X-macro: field name, C type, Ruby wrap/unwrap macros               */
/* ------------------------------------------------------------------ */

#define INT32_FIELD(name)   FIELD(name, int32_t,  NUM2INT,  INT2NUM)
#define INT64_FIELD(name)   FIELD(name, int64_t,  NUM2LL,  LL2NUM)
#define UINT64_FIELD(name)  FIELD(name, uint64_t, NUM2ULL, ULL2NUM)
#define FLOAT_FIELD(name)   FIELD(name, float,    NUM2DBL, DBL2NUM)

#define FIELD(name, ctype, unwrap, wrap)                              \
    static VALUE                                                      \
    mb_opt_get_##name(VALUE self)                                     \
    {                                                                 \
        mb_opt_t *opt = rb_minibwa_get_opt(self);                     \
        return wrap(opt->name);                                       \
    }                                                                 \
    static VALUE                                                      \
    mb_opt_set_##name(VALUE self, VALUE val)                          \
    {                                                                 \
        mb_opt_t *opt = rb_minibwa_get_opt(self);                     \
        opt->name = (ctype)unwrap(val);                               \
        return val;                                                   \
    }

/* ---- flag (uint64_t, special: also has bit predicates/setters) ---- */
UINT64_FIELD(flag)

/* ---- seeding options ---- */
INT32_FIELD(min_len)
INT32_FIELD(max_sub_occ)
INT32_FIELD(max_occ)

/* ---- general algorithm options ---- */
INT32_FIELD(bw)
INT32_FIELD(bw_long)
INT32_FIELD(max_gap)
INT32_FIELD(max_sr_len)

/* ---- chaining options ---- */
INT32_FIELD(max_chain_skip)
INT32_FIELD(max_chain_iter)
INT32_FIELD(min_chain_score)
FLOAT_FIELD(chain_gap_scale)

/* ---- hit processing options ---- */
FLOAT_FIELD(mask_level)
INT32_FIELD(mask_len)
FLOAT_FIELD(pri_ratio)
INT32_FIELD(best_n)

/* ---- alignment options ---- */
INT32_FIELD(a)
INT32_FIELD(b)
INT32_FIELD(b_ts)
INT32_FIELD(b_ambi)
INT32_FIELD(q)
INT32_FIELD(q2)
INT32_FIELD(e)
INT32_FIELD(e2)
INT32_FIELD(end_bonus)
INT32_FIELD(min_dp_max)
INT32_FIELD(zdrop)
INT32_FIELD(zdrop_inv)
INT32_FIELD(min_ksw_len)

/* ---- pairing options ---- */
INT32_FIELD(max_pe_ins)
INT32_FIELD(max_rescue)
INT32_FIELD(pen_unpair)
INT32_FIELD(pe_avg)
INT32_FIELD(pe_std)
INT32_FIELD(pe_lo)
INT32_FIELD(pe_hi)

/* ---- input/output options ---- */
INT32_FIELD(sb_len)
INT32_FIELD(sb_seq)
INT32_FIELD(n_thread)
INT32_FIELD(out_n)
FLOAT_FIELD(out_s)
INT32_FIELD(seed)
INT32_FIELD(xa_max)
INT64_FIELD(mb_size)
INT64_FIELD(max_mb_size)
INT64_FIELD(max_sw_mat)
INT64_FIELD(cap_kalloc)

#undef FIELD
#undef INT32_FIELD
#undef INT64_FIELD
#undef UINT64_FIELD
#undef FLOAT_FIELD

/* ------------------------------------------------------------------ */
/* Flag bit predicates and setters                                    */
/* ------------------------------------------------------------------ */

#define FLAG_BIT(name, bit)                                            \
    static VALUE                                                      \
    mb_opt_##name##_p(VALUE self)                                     \
    {                                                                 \
        mb_opt_t *opt = rb_minibwa_get_opt(self);                     \
        return (opt->flag & bit) ? Qtrue : Qfalse;                    \
    }                                                                 \
    static VALUE                                                      \
    mb_opt_set_##name(VALUE self, VALUE val)                          \
    {                                                                 \
        mb_opt_t *opt = rb_minibwa_get_opt(self);                     \
        if (RTEST(val)) opt->flag |= bit;                             \
        else           opt->flag &= ~bit;                              \
        return val;                                                   \
    }

FLAG_BIT(paf,           MB_F_PAF)
FLAG_BIT(no_unmap,      MB_F_NO_UNMAP)
FLAG_BIT(copy_comment,  MB_F_COPY_COMMENT)
FLAG_BIT(pe,            MB_F_PE)
FLAG_BIT(long_mode,     MB_F_LONG)
FLAG_BIT(eqx,           MB_F_EQX)
FLAG_BIT(no_kalloc,     MB_F_NO_KALLOC)
FLAG_BIT(no_aln,        MB_F_NO_ALN)
FLAG_BIT(pe_predef,     MB_F_PE_PREDEF)
FLAG_BIT(write_ds,      MB_F_WRITE_DS)
FLAG_BIT(write_cs,      MB_F_WRITE_CS)
FLAG_BIT(write_md,      MB_F_WRITE_MD)
FLAG_BIT(second_seq,    MB_F_2ND_SEQ)
FLAG_BIT(supp_soft,     MB_F_SUPP_SOFT)
FLAG_BIT(adap,          MB_F_ADAP)
FLAG_BIT(primary5,      MB_F_PRIMARY5)
FLAG_BIT(no_pairing,    MB_F_NO_PAIRING)
FLAG_BIT(meth,          MB_F_METH)

#undef FLAG_BIT

/* ------------------------------------------------------------------ */
/* Constructor, init, preset                                          */
/* ------------------------------------------------------------------ */

static VALUE
mb_opt_alloc(VALUE klass)
{
    mb_opt_t *opt;
    VALUE obj = TypedData_Make_Struct(klass, mb_opt_t,
                                      &rb_minibwa_options_type, opt);
    mb_opt_init(opt);
    return obj;
}

/*
 * call-seq:
 *   Options.new(**kwargs) -> Options
 *
 * Returns a new Options initialised with mb_opt_init() (adaptive paired-end
 * defaults).  Pass keyword arguments to override individual fields, e.g.
 *
 *   Options.new(bw: 500, min_len: 19)
 */
static VALUE
mb_opt_initialize(int argc, VALUE *argv, VALUE self)
{
    VALUE kwargs;
    rb_scan_args(argc, argv, ":", &kwargs);

    if (!NIL_P(kwargs)) {
        static ID id_keyword_new;
        if (!id_keyword_new)
            id_keyword_new = rb_intern("__keyword_new__");
        /* Delegate to Ruby-side keyword handling in lib/minibwa/options.rb */
        rb_funcall(self, id_keyword_new, 1, kwargs);
    }
    return Qnil;
}

/*
 * call-seq:
 *   preset(name) -> true or false
 *
 * Applies a named preset.  Returns +true+ on success, +false+ if the preset
 * name is unknown.  Known presets: "sr", "adap", "lr".
 */
static VALUE
rb_minibwa_opt_preset(VALUE self, VALUE name)
{
    mb_opt_t *opt = rb_minibwa_get_opt(self);
    const char *s = StringValueCStr(name);
    return mb_opt_preset(opt, s) == 0 ? Qtrue : Qfalse;
}

/* ------------------------------------------------------------------ */
/* Registration                                                       */
/* ------------------------------------------------------------------ */

#define DEF_ACCESSOR(klass, name)                                      \
    rb_define_method(klass, #name, mb_opt_get_##name, 0);             \
    rb_define_method(klass, #name "=", mb_opt_set_##name, 1);

#define DEF_FLAG(klass, name)                                          \
    rb_define_method(klass, #name "?", mb_opt_##name##_p, 0);         \
    rb_define_method(klass, #name "=", mb_opt_set_##name, 1);

void
rb_minibwa_init_options(void)
{
    VALUE klass = rb_define_class_under(rb_mMinibwa, "Options", rb_cObject);
    rb_cMinibwaOptions = klass;

    rb_define_alloc_func(klass, mb_opt_alloc);
    rb_define_method(klass, "initialize", mb_opt_initialize, -1);
    rb_define_method(klass, "preset!", rb_minibwa_opt_preset, 1);

    /* ---- scalar fields ---- */
    DEF_ACCESSOR(klass, flag)
    DEF_ACCESSOR(klass, min_len)
    DEF_ACCESSOR(klass, max_sub_occ)
    DEF_ACCESSOR(klass, max_occ)
    DEF_ACCESSOR(klass, bw)
    DEF_ACCESSOR(klass, bw_long)
    DEF_ACCESSOR(klass, max_gap)
    DEF_ACCESSOR(klass, max_sr_len)
    DEF_ACCESSOR(klass, max_chain_skip)
    DEF_ACCESSOR(klass, max_chain_iter)
    DEF_ACCESSOR(klass, min_chain_score)
    DEF_ACCESSOR(klass, chain_gap_scale)
    DEF_ACCESSOR(klass, mask_level)
    DEF_ACCESSOR(klass, mask_len)
    DEF_ACCESSOR(klass, pri_ratio)
    DEF_ACCESSOR(klass, best_n)
    DEF_ACCESSOR(klass, a)
    DEF_ACCESSOR(klass, b)
    DEF_ACCESSOR(klass, b_ts)
    DEF_ACCESSOR(klass, b_ambi)
    DEF_ACCESSOR(klass, q)
    DEF_ACCESSOR(klass, q2)
    DEF_ACCESSOR(klass, e)
    DEF_ACCESSOR(klass, e2)
    DEF_ACCESSOR(klass, end_bonus)
    DEF_ACCESSOR(klass, min_dp_max)
    DEF_ACCESSOR(klass, zdrop)
    DEF_ACCESSOR(klass, zdrop_inv)
    DEF_ACCESSOR(klass, min_ksw_len)
    DEF_ACCESSOR(klass, max_pe_ins)
    DEF_ACCESSOR(klass, max_rescue)
    DEF_ACCESSOR(klass, pen_unpair)
    DEF_ACCESSOR(klass, pe_avg)
    DEF_ACCESSOR(klass, pe_std)
    DEF_ACCESSOR(klass, pe_lo)
    DEF_ACCESSOR(klass, pe_hi)
    DEF_ACCESSOR(klass, sb_len)
    DEF_ACCESSOR(klass, sb_seq)
    DEF_ACCESSOR(klass, n_thread)
    DEF_ACCESSOR(klass, out_n)
    DEF_ACCESSOR(klass, out_s)
    DEF_ACCESSOR(klass, seed)
    DEF_ACCESSOR(klass, xa_max)
    DEF_ACCESSOR(klass, mb_size)
    DEF_ACCESSOR(klass, max_mb_size)
    DEF_ACCESSOR(klass, max_sw_mat)
    DEF_ACCESSOR(klass, cap_kalloc)

    /* ---- flag bits ---- */
    DEF_FLAG(klass, paf)
    DEF_FLAG(klass, no_unmap)
    DEF_FLAG(klass, copy_comment)
    DEF_FLAG(klass, pe)
    DEF_FLAG(klass, long_mode)
    DEF_FLAG(klass, eqx)
    DEF_FLAG(klass, no_kalloc)
    DEF_FLAG(klass, no_aln)
    DEF_FLAG(klass, pe_predef)
    DEF_FLAG(klass, write_ds)
    DEF_FLAG(klass, write_cs)
    DEF_FLAG(klass, write_md)
    DEF_FLAG(klass, second_seq)
    DEF_FLAG(klass, supp_soft)
    DEF_FLAG(klass, adap)
    DEF_FLAG(klass, primary5)
    DEF_FLAG(klass, no_pairing)
    DEF_FLAG(klass, meth)
}

#undef DEF_ACCESSOR
#undef DEF_FLAG
