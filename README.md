# ruby-minibwa

[![Gem Version](https://img.shields.io/gem/v/minibwa?color=brightgreen)](https://rubygems.org/gems/minibwa)
[![Test](https://github.com/kojix2/ruby-minibwa/actions/workflows/test.yml/badge.svg)](https://github.com/kojix2/ruby-minibwa/actions/workflows/test.yml)
[![Lines of Code](https://img.shields.io/endpoint?url=https%3A%2F%2Ftokei.kojix2.net%2Fbadge%2Fgithub%2Fkojix2%2Fruby-minibwa%2Flines)](https://tokei.kojix2.net/github/kojix2/ruby-minibwa)
[![DOI](https://zenodo.org/badge/1319741104.svg)](https://doi.org/10.5281/zenodo.21753646)

Ruby bindings for [minibwa](https://github.com/lh3/minibwa), a short-read
aligner combining bwa-mem seeding with minimap2 chaining and alignment.

## Installation

Requires a C compiler, zlib, and Ruby >= 3.2.

```sh
git clone --recursive https://github.com/kojix2/ruby-minibwa
cd ruby-minibwa
bundle install
bundle exec rake compile
```

The extension builds the vendored minibwa sources, including libsais for
index construction. Expect the first build to take a few minutes.

## Usage

Build an index:

```ruby
require "minibwa"

Minibwa::Index.build("ref.fa", "ref", n_thread: 4)
```

This writes `ref.l2b` and `ref.mbw`. Indexes made by the `minibwa index`
command are also usable.

Map reads:

```ruby
opt = Minibwa::Options.new
opt.preset("sr")  # or "adap", "lr"

idx = Minibwa::Index.load("ref")

hits = idx.map("ACGT...", name: "read1", opt: opt)
hits.each do |h|
  puts [h.ctg, h.ts, h.te, h.strand, h.qs, h.qe, h.mapq, h.cigar_str].join("\t")
end
```

Map many reads in one call:

```ruby
seqs  = ["ACGT...", "TGCA..."]
names = ["read1", "read2"]
hits  = idx.map_batch(seqs, names: names, opt: opt)
# => [[Hit, ...], [Hit, ...]]
```

Use a reusable buffer for repeated mapping:

```ruby
buf = Minibwa::Buffer.new
idx.map(seq, buf: buf)
```

Generate SAM output:

```ruby
puts Minibwa::Sam.header(idx)
hits.each do |h|
  puts Minibwa::Sam.format("read1", 0, h, seq)
end
```

**Note on reverse-strand alignments:** For hits where `hit.rev` is true,
the SAM spec requires SEQ to be the reverse complement of the original
query and the FLAG to have 0x10 set. `Sam.format` does not do this
automatically — pass the already-oriented sequence and the correct flag.
The library does not retain the original query after mapping.

With `pe: true`, `map_batch` treats consecutive sequences as read pairs.

### CIGAR

`Hit#cigar` and `#cigar_str` cover the aligned region only and contain no
clipping, which is how minibwa reports alignments. `#full_cigar_str`
takes the query length and adds soft clips derived from `qs` and `qe`.

### Threads

`Options#n_thread` affects only `Index.build` (via libsais + OpenMP).
Mapping itself is single-threaded per call, but releases the GVL, so Ruby
threads run in parallel. To map from several Ruby threads, give each one
its own buffer:

```ruby
buf = Minibwa::Buffer.new
idx.map(seq, opt: opt, buf: buf)
```

A `Buffer` holds mutable scratch state and must not be shared between
threads mapping at the same time.

## Development

```sh
bundle exec rake compile
bundle exec rake test
```

`bin/console` gives an IRB session with the extension loaded.

## License

MIT. The vendored minibwa is MIT as well; its GPL-licensed files
(`bwtgen.c`, `QSufSort.c`, providing the low-memory BWT construction
option) are neither compiled nor distributed. See
`ext/minibwa/minibwa/LICENSE.txt`.
