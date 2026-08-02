# frozen_string_literal: true

module Minibwa
  # Alignment parameters, a Ruby view of mb_opt_t.
  #
  # The class and all of its field accessors come from mb_options.c; this file
  # reopens it for what reads better in Ruby -- keyword initialization,
  # #to_h and #inspect, and rejecting an unknown preset name before it reaches
  # mb_opt_preset().
  #
  # Presets are "sr", "adap" and "lr". Note that mb_opt_init() applies "adap",
  # so a fresh Options is paired-end and adaptive by default.
  class Options
    PRESETS = %w[sr adap lr].freeze

    # Applies a named preset with validation.
    #
    #   opt.preset("sr")   # => true
    #   opt.preset("bad")  # raises ArgumentError
    def preset(name)
      unless PRESETS.include?(name.to_s)
        raise ArgumentError, "unknown preset: #{name.inspect}, expected one of #{PRESETS.inspect}"
      end

      preset!(name)
    end

    # Returns all field values as a Hash.
    def to_h
      {
        flag: flag, min_len: min_len, max_sub_occ: max_sub_occ, max_occ: max_occ,
        bw: bw, bw_long: bw_long, max_gap: max_gap, max_sr_len: max_sr_len,
        max_chain_skip: max_chain_skip, max_chain_iter: max_chain_iter,
        min_chain_score: min_chain_score, chain_gap_scale: chain_gap_scale,
        mask_level: mask_level, mask_len: mask_len, pri_ratio: pri_ratio,
        best_n: best_n, a: a, b: b, b_ts: b_ts, b_ambi: b_ambi,
        q: q, q2: q2, e: e, e2: e2, end_bonus: end_bonus,
        min_dp_max: min_dp_max, zdrop: zdrop, zdrop_inv: zdrop_inv,
        min_ksw_len: min_ksw_len, max_pe_ins: max_pe_ins, max_rescue: max_rescue,
        pen_unpair: pen_unpair, pe_avg: pe_avg, pe_std: pe_std,
        pe_lo: pe_lo, pe_hi: pe_hi, sb_len: sb_len, sb_seq: sb_seq,
        n_thread: n_thread, out_n: out_n, out_s: out_s, seed: seed,
        xa_max: xa_max, mb_size: mb_size, max_mb_size: max_mb_size,
        max_sw_mat: max_sw_mat, cap_kalloc: cap_kalloc
      }
    end

    def inspect
      "#<#{self.class.name} #{to_h.map { |k, v| "#{k}=#{v.inspect}" }.join(', ')}>"
    end

    private

    # Called from C's initialize with a keyword-arguments Hash.
    # @api private
    def __keyword_new__(kwargs)
      kwargs.each do |key, value|
        setter = :"#{key}="
        raise ArgumentError, "unknown option: #{key}" unless respond_to?(setter)

        public_send(setter, value)
      end
    end
  end
end
