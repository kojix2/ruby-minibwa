# frozen_string_literal: true

require 'test_helper'

# Minibwa::Sam: header lines from an index, alignment lines from Hit objects,
# and in particular the soft clips reconstructed from qs/qe, which upstream
# leaves out of the CIGAR.
class SamTest < Test::Unit::TestCase
  FASTA = File.expand_path('fixtures/ref.fa', __dir__)
  PREFIX = File.expand_path('fixtures/ref', __dir__)

  def setup
    Minibwa::Index.build(FASTA, PREFIX)
    @idx = Minibwa::Index.load(PREFIX)
  end

  test 'header includes @SQ lines' do
    hdr = Minibwa::Sam.header(@idx)
    assert_match(/@HD\tVN:1.6/, hdr)
    assert_match(/@SQ\tSN:chr1\tLN:280/, hdr)
  end

  test 'format produces a SAM line' do
    seq = 'GATTACAGATTACAGATTACAGATTACAGATTACA'
    hit = @idx.map(seq, name: 'read1').first
    line = Minibwa::Sam.format('read1', 0, hit, seq)
    fields = line.split("\t")
    assert_equal('read1', fields[0])
    assert_equal('0', fields[1])
    assert_equal('chr1', fields[2])
    assert_equal('35M', fields[5])
    assert_equal(seq, fields[9])
  end

  test 'soft clips are reconstructed' do
    # A query with a non-matching suffix should be soft-clipped.  The
    # reference is a GATTACA repeat, so trailing "C"s cannot align and are
    # soft-clipped rather than mismatched.
    seq = 'GATTACAGATTACAGATTACAGATTACAGATTACACCC'
    hit = @idx.map(seq, name: 'read1').first
    refute_nil(hit, "expected a hit for #{seq}")
    cigar = Minibwa::Sam.build_full_cigar(hit, seq.length)
    assert_equal('35M3S', cigar)
  end

  test 'full_cigar_str takes query length' do
    seq = 'GATTACAGATTACAGATTACAGATTACAGATTACACCC'
    hit = @idx.map(seq, name: 'read1').first
    assert_equal('35M3S', hit.full_cigar_str(seq.length))
  end

  test 'NM tag is the edit distance' do
    seq = 'GATTACAGATTACAGATTACAGATTACAGATTACA'
    hit = @idx.map(seq, name: 'read1').first
    line = Minibwa::Sam.format('read1', 0, hit, seq)
    nm = line[/NM:i:(\d+)/, 1].to_i
    assert_equal(hit.blen - hit.mlen, nm)
  end

  test 'mate formats as = when same contig' do
    seq = 'GATTACAGATTACAGATTACAGATTACAGATTACA'
    hit = @idx.map(seq, name: 'read1').first
    mate = hit # same contig for testing
    line = Minibwa::Sam.format('read1', 0, hit, seq, mate: mate)
    fields = line.split("\t")
    assert_equal('=', fields[6])
  end
end
