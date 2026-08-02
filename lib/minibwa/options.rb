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
  #
  # @!attribute [rw] flag
  #   @return [Integer] raw bit flags.
  # @!attribute [rw] min_len
  #   @return [Integer] minimum seed length.
  # @!attribute [rw] max_sub_occ
  #   @return [Integer] maximum occurrence count for sub-seeds.
  # @!attribute [rw] max_occ
  #   @return [Integer] maximum occurrence count for seeds.
  # @!attribute [rw] bw
  #   @return [Integer] band width for dynamic programming.
  # @!attribute [rw] bw_long
  #   @return [Integer] band width for long alignments.
  # @!attribute [rw] max_gap
  #   @return [Integer] maximum gap size considered during chaining.
  # @!attribute [rw] max_sr_len
  #   @return [Integer] maximum short-read length.
  # @!attribute [rw] max_chain_skip
  #   @return [Integer] maximum skipped anchors during chaining.
  # @!attribute [rw] max_chain_iter
  #   @return [Integer] maximum chaining iterations.
  # @!attribute [rw] min_chain_score
  #   @return [Integer] minimum chaining score.
  # @!attribute [rw] chain_gap_scale
  #   @return [Float] gap penalty scale for chaining.
  # @!attribute [rw] mask_level
  #   @return [Float] masking level for secondary hits.
  # @!attribute [rw] mask_len
  #   @return [Integer] masking length threshold.
  # @!attribute [rw] pri_ratio
  #   @return [Float] primary-to-secondary score ratio.
  # @!attribute [rw] best_n
  #   @return [Integer] number of best hits retained.
  # @!attribute [rw] a
  #   @return [Integer] match score.
  # @!attribute [rw] b
  #   @return [Integer] mismatch penalty.
  # @!attribute [rw] b_ts
  #   @return [Integer] transition mismatch penalty.
  # @!attribute [rw] b_ambi
  #   @return [Integer] ambiguous-base mismatch penalty.
  # @!attribute [rw] q
  #   @return [Integer] gap-open penalty.
  # @!attribute [rw] q2
  #   @return [Integer] second gap-open penalty.
  # @!attribute [rw] e
  #   @return [Integer] gap-extension penalty.
  # @!attribute [rw] e2
  #   @return [Integer] second gap-extension penalty.
  # @!attribute [rw] end_bonus
  #   @return [Integer] alignment end bonus.
  # @!attribute [rw] min_dp_max
  #   @return [Integer] minimum dynamic-programming score.
  # @!attribute [rw] zdrop
  #   @return [Integer] Z-drop threshold.
  # @!attribute [rw] zdrop_inv
  #   @return [Integer] inversion Z-drop threshold.
  # @!attribute [rw] min_ksw_len
  #   @return [Integer] minimum length for Smith-Waterman alignment.
  # @!attribute [rw] max_pe_ins
  #   @return [Integer] maximum paired-end insert size.
  # @!attribute [rw] max_rescue
  #   @return [Integer] maximum rescue attempts.
  # @!attribute [rw] pen_unpair
  #   @return [Integer] unpaired alignment penalty.
  # @!attribute [rw] pe_avg
  #   @return [Integer] paired-end insert-size average.
  # @!attribute [rw] pe_std
  #   @return [Integer] paired-end insert-size standard deviation.
  # @!attribute [rw] pe_lo
  #   @return [Integer] paired-end lower insert-size bound.
  # @!attribute [rw] pe_hi
  #   @return [Integer] paired-end upper insert-size bound.
  # @!attribute [rw] sb_len
  #   @return [Integer] sequence batch length.
  # @!attribute [rw] sb_seq
  #   @return [Integer] sequence batch count.
  # @!attribute [rw] n_thread
  #   @return [Integer] number of threads for batch work.
  # @!attribute [rw] out_n
  #   @return [Integer] maximum number of output alignments.
  # @!attribute [rw] out_s
  #   @return [Float] minimum output score ratio.
  # @!attribute [rw] seed
  #   @return [Integer] random seed.
  # @!attribute [rw] xa_max
  #   @return [Integer] maximum XA tag hits.
  # @!attribute [rw] mb_size
  #   @return [Integer] mini-batch size.
  # @!attribute [rw] max_mb_size
  #   @return [Integer] maximum mini-batch size.
  # @!attribute [rw] max_sw_mat
  #   @return [Integer] maximum Smith-Waterman matrix size.
  # @!attribute [rw] cap_kalloc
  #   @return [Integer] kalloc capacity limit.
  #
  # @!method paf?
  #   @return [Boolean] whether PAF output mode is enabled.
  # @!method paf=(value)
  #   @return [Object] value
  # @!method no_unmap?
  #   @return [Boolean] whether unmapped records are suppressed.
  # @!method no_unmap=(value)
  #   @return [Object] value
  # @!method copy_comment?
  #   @return [Boolean] whether FASTQ comments are copied.
  # @!method copy_comment=(value)
  #   @return [Object] value
  # @!method pe?
  #   @return [Boolean] whether paired-end mode is enabled.
  # @!method pe=(value)
  #   @return [Object] value
  # @!method long_mode?
  #   @return [Boolean] whether long-read mode is enabled.
  # @!method long_mode=(value)
  #   @return [Object] value
  # @!method eqx?
  #   @return [Boolean] whether CIGAR uses =/X operators.
  # @!method eqx=(value)
  #   @return [Object] value
  # @!method no_kalloc?
  #   @return [Boolean] whether kalloc is disabled.
  # @!method no_kalloc=(value)
  #   @return [Object] value
  # @!method no_aln?
  #   @return [Boolean] whether alignment is disabled.
  # @!method no_aln=(value)
  #   @return [Object] value
  # @!method pe_predef?
  #   @return [Boolean] whether paired-end bounds are predefined.
  # @!method pe_predef=(value)
  #   @return [Object] value
  # @!method write_ds?
  #   @return [Boolean] whether the DS tag is emitted.
  # @!method write_ds=(value)
  #   @return [Object] value
  # @!method write_cs?
  #   @return [Boolean] whether the CS tag is emitted.
  # @!method write_cs=(value)
  #   @return [Object] value
  # @!method write_md?
  #   @return [Boolean] whether the MD tag is emitted.
  # @!method write_md=(value)
  #   @return [Object] value
  # @!method second_seq?
  #   @return [Boolean] whether second-sequence output is enabled.
  # @!method second_seq=(value)
  #   @return [Object] value
  # @!method supp_soft?
  #   @return [Boolean] whether supplementary alignments use soft clipping.
  # @!method supp_soft=(value)
  #   @return [Object] value
  # @!method adap?
  #   @return [Boolean] whether adaptive mode is enabled.
  # @!method adap=(value)
  #   @return [Object] value
  # @!method primary5?
  #   @return [Boolean] whether primary alignment selection is 5-prime based.
  # @!method primary5=(value)
  #   @return [Object] value
  # @!method no_pairing?
  #   @return [Boolean] whether pairing is disabled.
  # @!method no_pairing=(value)
  #   @return [Object] value
  # @!method meth?
  #   @return [Boolean] whether methylation mode is enabled.
  # @!method meth=(value)
  #   @return [Object] value
  class Options
    # Supported preset names for {#preset}.
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

    # Returns a compact representation of all option fields.
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
