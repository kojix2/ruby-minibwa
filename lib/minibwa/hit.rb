# frozen_string_literal: true

module Minibwa
  # A single alignment of one query against the reference.
  #
  # The fields are filled in by mb_hit.c. This file reopens the class for the
  # values that can be derived without touching C: CIGAR formatting, the
  # soft-clipped CIGAR rebuilt from qs/qe -- upstream only reports the aligned
  # region -- the strand character, and #inspect.
  class Hit
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

    def inspect
      attrs = %i[tid ctg ts te qs qe strand score mapq mlen blen]
      "#<#{self.class.name} #{attrs.map { |a| "#{a}=#{send(a).inspect}" }.join(' ')}>"
    end
  end
end
