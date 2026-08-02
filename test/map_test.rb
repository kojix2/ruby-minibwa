# frozen_string_literal: true

require 'test_helper'

# Mapping: Index#map and #map_batch, the Hit fields and CIGAR they produce,
# agreement between the single and batch paths, and reuse of a Buffer.
class MapTest < Test::Unit::TestCase
  FASTA = File.expand_path('fixtures/ref.fa', __dir__)
  PREFIX = File.expand_path('fixtures/ref', __dir__)

  def setup
    Minibwa::Index.build(FASTA, PREFIX)
    @idx = Minibwa::Index.load(PREFIX)
  end

  test 'map returns Hit objects' do
    hits = @idx.map('GATTACAGATTACAGATTACAGATTACAGATTACA', name: 'read1')
    refute_empty(hits)
    hits.each do |h|
      assert_instance_of(Minibwa::Hit, h)
      assert_equal('chr1', h.ctg)
      assert_equal(0, h.tid)
      assert(h.score > 0)
      assert_equal('35M', h.cigar_str)
    end
  end

  test 'map_batch agrees with map' do
    seqs = %w[GATTACAGATTACAGATTACAGATTACAGATTACA TTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTT]
    names = %w[read1 read2]
    batch = @idx.map_batch(seqs, names: names)

    assert_equal(2, batch.size)
    single = @idx.map(seqs[0], name: names[0])
    assert_equal(single.size, batch[0].size)
    assert_equal(single.first.cigar_str, batch[0].first.cigar_str)
    assert_equal(single.first.ts, batch[0].first.ts)
  end

  test 'map_batch validates names' do
    seqs = %w[GATTACAGATTACAGATTACAGATTACAGATTACA TTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTT]

    assert_raise(TypeError) { @idx.map_batch(seqs, names: 'read1') }
    assert_raise(ArgumentError) { @idx.map_batch(seqs, names: ['read1']) }
  end

  test 'Buffer reuse' do
    buf = Minibwa::Buffer.new
    hits1 = @idx.map('GATTACAGATTACAGATTACAGATTACAGATTACA', buf: buf)
    hits2 = @idx.map('GATTACAGATTACAGATTACAGATTACAGATTACA', buf: buf)
    assert_equal(hits1.size, hits2.size)
  end

  test 'unmapped query returns empty array' do
    hits = @idx.map('TTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTT')
    assert_empty(hits)
  end

  test 'Hit strand' do
    # GATTACA is not self-complementary, so the forward strand is the only
    # one that matches the reference.
    hits = @idx.map('GATTACAGATTACAGATTACAGATTACAGATTACA')
    strands = hits.map(&:strand).uniq
    assert_equal(['+'], strands)
  end

  test 'map_batch with meth mode on non-meth index raises' do
    opt = Minibwa::Options.new
    opt.meth = true
    assert_raise(Minibwa::Error) do
      @idx.map_batch(['GATTACAGATTACA'], opt: opt)
    end
  end
end
