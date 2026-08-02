# frozen_string_literal: true

module Minibwa
  # A single alignment of one query against the reference.
  #
  # The fields are filled in by mb_hit.c. This file reopens the class for the
  # values that can be derived without touching C: CIGAR formatting, the
  # soft-clipped CIGAR rebuilt from qs/qe -- upstream only reports the aligned
  # region -- the strand character, and #inspect.
  #
  # @!attribute [r] tid
  #   @return [Integer] target contig ID.
  # @!attribute [r] ts
  #   @return [Integer] zero-based target start.
  # @!attribute [r] te
  #   @return [Integer] zero-based target end.
  # @!attribute [r] qs
  #   @return [Integer] zero-based query start.
  # @!attribute [r] qe
  #   @return [Integer] zero-based query end.
  # @!attribute [r] score
  #   @return [Integer] alignment score.
  # @!attribute [r] score0
  #   @return [Integer] best-chain score before post-processing.
  # @!attribute [r] mlen
  #   @return [Integer] number of matching bases.
  # @!attribute [r] blen
  #   @return [Integer] alignment block length.
  # @!attribute [r] mapq
  #   @return [Integer] mapping quality.
  # @!attribute [r] cnt
  #   @return [Integer] number of seed hits.
  # @!attribute [r] n_sub
  #   @return [Integer] number of suboptimal hits.
  # @!attribute [r] subsc
  #   @return [Integer] suboptimal alignment score.
  # @!attribute [r] hash
  #   @return [Integer] upstream hash value for tie-breaking.
  # @!attribute [r] rev
  #   @return [Boolean] whether the hit is on the reverse strand.
  # @!attribute [r] proper_pair
  #   @return [Boolean] whether the hit is part of a proper pair.
  # @!attribute [r] sam_pri
  #   @return [Boolean] whether the hit is primary in SAM output.
  # @!attribute [r] flt
  #   @return [Boolean] whether the hit is filtered.
  # @!attribute [r] inv
  #   @return [Boolean] whether the hit represents an inversion.
  # @!attribute [r] split
  #   @return [Boolean] whether the hit is split.
  # @!attribute [r] split_inv
  #   @return [Boolean] whether the split hit is inverted.
  # @!attribute [r] rescued
  #   @return [Boolean] whether paired-end rescue found the hit.
  # @!attribute [r] frac_high
  #   @return [Boolean] whether the high-frequency seed fraction is high.
  # @!attribute [r] seed_ratio
  #   @return [Boolean] whether the seed ratio flag is set.
  # @!attribute [r] ctg
  #   @return [String, nil] target contig name, when available.
  # @!attribute [r] dp_score
  #   @return [Integer] dynamic-programming score.
  # @!attribute [r] dp_max0
  #   @return [Integer] first dynamic-programming maximum.
  # @!attribute [r] dp_max
  #   @return [Integer] dynamic-programming maximum.
  # @!attribute [r] dp_max2
  #   @return [Integer] second dynamic-programming maximum.
  # @!attribute [r] n_ambi
  #   @return [Integer] number of ambiguous reference bases.
  # @!attribute [r] cs_flag
  #   @return [Integer] CS tag flags.
  # @!attribute [r] cigar
  #   @return [Array<Array(Integer, Integer)>] CIGAR operations as [length, op] pairs.
  class Hit
    # CIGAR operation characters indexed by operation code.
    CIGAR_STR = 'MIDNSHP=XB'

    # Returns the CIGAR string for the aligned region (no soft/hard clips).
    def cigar_str
      @cigar.map { |len, op| "#{len}#{CIGAR_STR[op]}" }.join
    end

    # Returns the full CIGAR string including soft clips.
    #
    # The query length is required because the Hit only stores the aligned
    # region (qs/qe); the clips are derived from the difference.
    #
    #   hit.full_cigar_str(query.length)  # => "2S35M3S"
    def full_cigar_str(query_len)
      Sam.build_full_cigar(self, query_len)
    end

    # Returns the strand as '+' or '-'.
    def strand
      rev ? '-' : '+'
    end

    # Returns the target (reference) span length.
    def tlen
      te - ts
    end

    # Returns a compact representation of the most commonly inspected fields.
    def inspect
      attrs = %i[tid ctg ts te qs qe strand score mapq mlen blen]
      "#<#{self.class.name} #{attrs.map { |a| "#{a}=#{send(a).inspect}" }.join(' ')}>"
    end
  end
end
