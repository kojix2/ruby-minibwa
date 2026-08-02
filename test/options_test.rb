# frozen_string_literal: true

require 'test_helper'

# Minibwa::Options: the mb_opt_init() defaults, each preset, and a read/write
# round trip over every generated accessor and MB_F_* flag.
class OptionsTest < Test::Unit::TestCase
  def setup
    @opt = Minibwa::Options.new
  end

  test 'defaults from mb_opt_init' do
    assert_equal(19, @opt.min_len)
    assert_equal(100, @opt.bw)
    assert_equal(2, @opt.a)
    assert_equal(8, @opt.b)
    assert_equal(12, @opt.q)
    assert_equal(2, @opt.e)
    assert_equal(11, @opt.seed)
    assert_equal(1, @opt.n_thread)
  end

  test 'adaptive mode is the default' do
    assert(@opt.adap?)
  end

  test 'accessor round trip' do
    @opt.min_len = 15
    @opt.bw = 500
    @opt.a = 1
    @opt.b = 4
    @opt.q = 6
    @opt.e = 1
    @opt.n_thread = 8
    @opt.seed = 42
    @opt.chain_gap_scale = 0.5

    assert_equal(15, @opt.min_len)
    assert_equal(500, @opt.bw)
    assert_equal(1, @opt.a)
    assert_equal(4, @opt.b)
    assert_equal(6, @opt.q)
    assert_equal(1, @opt.e)
    assert_equal(8, @opt.n_thread)
    assert_equal(42, @opt.seed)
    assert_in_delta(0.5, @opt.chain_gap_scale, 1e-6)
  end

  test 'flag bit predicates and setters' do
    assert_equal(false, @opt.paf?)
    @opt.paf = true
    assert(@opt.paf?)

    assert_equal(false, @opt.eqx?)
    @opt.eqx = true
    assert(@opt.eqx?)

    @opt.paf = false
    assert_equal(false, @opt.paf?)
  end

  test 'preset applies known presets' do
    %w[sr adap lr].each do |name|
      assert(@opt.preset(name), "expected preset #{name} to succeed")
    end
  end

  test 'unknown preset raises ArgumentError' do
    assert_raise(ArgumentError) { @opt.preset('bogus') }
  end

  test 'keyword initialization' do
    opt = Minibwa::Options.new(bw: 300, min_len: 21)
    assert_equal(300, opt.bw)
    assert_equal(21, opt.min_len)
  end

  test 'unknown keyword raises ArgumentError' do
    assert_raise(ArgumentError) { Minibwa::Options.new(bogus: 1) }
  end

  test 'to_h returns all fields' do
    h = @opt.to_h
    assert_equal(19, h[:min_len])
    assert_equal(100, h[:bw])
    assert_equal(11, h[:seed])
  end
end
