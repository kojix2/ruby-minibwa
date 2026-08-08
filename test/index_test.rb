# frozen_string_literal: true

require 'test_helper'
require 'fileutils'
require 'tmpdir'

# Minibwa::Index construction and loading: build an index from a FASTA,
# open it plainly and via mmap, read the contig metadata, and check that a
# missing index raises instead of aborting.
class IndexTest < Test::Unit::TestCase
  FASTA = File.expand_path('fixtures/ref.fa', __dir__)
  PREFIX = File.expand_path('fixtures/ref', __dir__)

  def setup
    Minibwa::Index.build(FASTA, PREFIX)
  end

  test 'build creates index files' do
    assert(File.exist?("#{PREFIX}.l2b"))
    assert(File.exist?("#{PREFIX}.mbw"))
  end

  test 'load returns an Index' do
    idx = Minibwa::Index.load(PREFIX)
    assert_instance_of(Minibwa::Index, idx)
  end

  test 'load_mmap returns an Index' do
    idx = Minibwa::Index.load_mmap(PREFIX)
    assert_instance_of(Minibwa::Index, idx)
  end

  test 'load_mmap with preload returns an Index' do
    idx = Minibwa::Index.load_mmap(PREFIX, preload: true)
    assert_instance_of(Minibwa::Index, idx)
  end

  test 'contig metadata' do
    idx = Minibwa::Index.load(PREFIX)
    assert_equal('chr1', idx.ctg_name(0))
    assert_equal(280, idx.ctg_len(0))
    assert_nil(idx.ctg_name(1))
    assert_nil(idx.ctg_len(1))
  end

  test 'missing index raises Minibwa::Error' do
    assert_raise(Minibwa::Error) do
      Minibwa::Index.load('test/fixtures/does_not_exist')
    end
  end

  test 'Index.new is not supported' do
    assert_raise(TypeError) { Minibwa::Index.new }
  end

  test 'build supports long output prefixes' do
    omit('long-path support depends on the Windows host') if Gem.win_platform?
    Dir.mktmpdir('minibwa-index-') do |dir|
      nested = dir
      9.times do |i|
        nested = File.join(nested, "segment-#{i}-#{'a' * 110}")
      end
      begin
        FileUtils.mkdir_p(nested)
      rescue Errno::ENAMETOOLONG
        omit('filesystem does not support the long path required by this test')
      end

      prefix = File.join(nested, 'ref')
      assert_operator(prefix.bytesize, :>, 1024)
      assert_equal(true, Minibwa::Index.build(FASTA, prefix))
      assert(File.exist?("#{prefix}.l2b"))
      assert(File.exist?("#{prefix}.mbw"))
    end
  end
end
