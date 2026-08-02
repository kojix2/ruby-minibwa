# frozen_string_literal: true

module Minibwa
  # SAM output built from Hit objects, in pure Ruby.
  #
  # Upstream's mb_fmt_sam() is deliberately not wrapped: it takes types that
  # live in the private mbpriv.h (l2b_t, mb_bseq1_t) and would tie the gem to
  # minibwa's internals. Doing it here also puts the soft clips that
  # Hit#cigar omits back into the emitted CIGAR.
  #
  # == Reverse-strand alignments
  #
  # For reverse-strand alignments (Hit#rev == true), the SAM spec requires
  # SEQ to be the reverse complement of the original query and the FLAG
  # to have 0x10 set.  Sam.format does NOT do this automatically: callers
  # must pass the already-oriented sequence and the correct flag.  This is
  # because the library does not retain the original query sequence after
  # mapping.
  module Sam
    module_function

    # Generates a SAM header from an Index.
    #
    #   puts Minibwa::Sam.header(index)
    def header(index, hd: nil, pg: nil, rg: nil)
      lines = [hd || "@HD\tVN:1.6\tSO:unsorted"]

      # Reference sequences from the index
      tid = 0
      while (name = index.ctg_name(tid))
        len = index.ctg_len(tid)
        lines << "@SQ\tSN:#{name}\tLN:#{len}"
        tid += 1
      end

      lines << pg if pg
      lines << rg if rg
      lines.join("\n") + "\n"
    end

    # Formats a single alignment as a SAM line.
    #
    # +qname+::  query name.
    # +flag+::   SAM flag (Integer).
    # +hit+::    a Minibwa::Hit.
    # +seq+::    the original query sequence.
    # +qual+::   the original quality string (or "*").
    # +mate+::   optional mate Hit for paired-end reads.
    #
    # Returns a SAM line String (without trailing newline).
    def format(qname, flag, hit, seq, qual = '*', mate: nil)
      rnext, pnext = if mate
        # '=' means the mate is on the same reference sequence.
        mate_ref = mate.ctg == hit.ctg && hit.ctg ? '=' : mate.ctg || '*'
        [mate_ref, mate.ts + 1]
      else
        ['*', 0]
      end

      # For reverse-strand alignments the SEQ field should be the reverse
      # complement of the original query (SAM spec).  Callers must pass the
      # already-oriented sequence; see the module documentation at the top
      # of this file.
      # NM is the edit distance, not the number of suboptimal alignments.
      # blen - mlen counts the non-matching positions within the alignment.
      [
        qname, flag, hit.ctg || '*', hit.ts + 1, hit.mapq,
        build_full_cigar(hit, seq.length), rnext, pnext, 0,
        seq || '*', qual || '*',
        "NM:i:#{hit.blen - hit.mlen}", "AS:i:#{hit.score}"
      ].join("\t")
    end

    # Builds the full CIGAR string including soft clips.
    def build_full_cigar(hit, query_len)
      parts = []
      parts << "#{hit.qs}S" if hit.qs > 0
      cigar = hit.cigar_str
      parts << cigar unless cigar.empty?
      soft_clip = query_len - hit.qe
      parts << "#{soft_clip}S" if soft_clip > 0
      parts.empty? ? '*' : parts.join
    end
  end
end
