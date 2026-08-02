# frozen_string_literal: true

require 'bundler/gem_tasks'
require 'rake/clean'
require 'rake/testtask'

Rake::TestTask.new(:test) do |t|
  t.libs << 'test'
  t.libs << 'lib'
  t.test_files = FileList['test/**/*_test.rb']
end

require 'rake/extensiontask'

task build: :compile

GEMSPEC = Gem::Specification.load('minibwa.gemspec')

Rake::ExtensionTask.new('minibwa', GEMSPEC) do |ext|
  ext.lib_dir = 'lib/minibwa'
end

CLEAN.include(
  'tmp', 'Makefile', 'mkmf.log',
  'ext/minibwa/Makefile', 'ext/minibwa/mkmf.log',
  'lib/minibwa/minibwa.{so,bundle,dll}',
  'test/fixtures/*.{l2b,mbw,bwt,pac,fai}'
)
CLOBBER.include('pkg', 'checksums', 'doc', '.yardoc')

task default: %i[clean compile test]
