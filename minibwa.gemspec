# frozen_string_literal: true

require_relative 'lib/minibwa/version'

Gem::Specification.new do |spec|
  spec.name = 'minibwa'
  spec.version = Minibwa::VERSION
  spec.authors = ['kojix2']
  spec.summary = 'Ruby bindings for minibwa, a short-read aligner'
  spec.homepage = 'https://github.com/kojix2/ruby-minibwa'
  spec.license = 'MIT'
  spec.required_ruby_version = '>= 3.2.0'

  upstream = %w[
    kommon kalloc bwt l2bit options seed map-algo lchain align pe cs format
    ksw2_extz2_sse ksw2_extd2_sse ksw2_ll_sse libsais libsais64 index
  ].map { |name| "ext/minibwa/minibwa/#{name}.c" }

  spec.files = %w[README.md LICENSE.txt ext/minibwa/minibwa/LICENSE.txt] +
               Dir.glob(['lib/**/*.rb', 'ext/minibwa/*.{c,h,rb}', 'ext/minibwa/compat/**/*.h',
                         'ext/minibwa/minibwa/*.h'], base: __dir__) +
               upstream
  spec.files.delete('ext/minibwa/minibwa/QSufSort.h')
  spec.extensions = ['ext/minibwa/extconf.rb']
end
