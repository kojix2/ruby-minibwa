# frozen_string_literal: true

module Minibwa
  # A minibwa index -- the FM-index plus the 2-bit reference -- and the object
  # you map reads against.
  #
  # The class comes from mb_index.c and mb_index_build.c; this file reopens it
  # for checking paths and permissions
  # before calling in. That check is not cosmetic: upstream reports bad input
  # to the index builder with kom_assert(), which abort()s the whole process
  # instead of returning an error.
  class Index
    # IMPORTANT: alias the raw C methods BEFORE redefining them.
    # Singleton methods (Index.load, etc.)
    class << self
      # @api private
      alias _build build
      # @api private
      alias _load load
      # @api private
      alias _load_mmap load_mmap
      private :_build, :_load, :_load_mmap
    end
    private_class_method :_build, :_load, :_load_mmap

    # Instance methods (index.map, index.map_batch)
    # @api private
    alias _map map
    # @api private
    alias _map_batch map_batch
    private :_map, :_map_batch

    class << self
      # Validates that the FASTA file exists and is readable before calling
      # the C extension (which would abort() on bad input).
      def build(fasta, prefix, **kwargs)
        raise Error, "cannot read FASTA file: #{fasta}" unless File.readable?(fasta)

        dir = File.dirname(prefix)
        raise Error, "cannot write to directory: #{dir}" unless File.writable?(dir)

        _build(fasta, prefix, **kwargs)
      end

      # Wraps Index.load with path validation.
      def load(prefix, **kwargs)
        check_prefix(prefix)
        _load(prefix, **kwargs)
      end

      # Wraps Index.load_mmap with path validation.
      def load_mmap(prefix, **kwargs)
        check_prefix(prefix)
        _load_mmap(prefix, **kwargs)
      end

      private

      def check_prefix(prefix)
        mbw = "#{prefix}.mbw"
        return if File.readable?(mbw)

        raise Error, "index not found: #{mbw}"
      end
    end

    # Convenience method: map a single sequence with keyword arguments.
    def map(seq, name: nil, opt: nil, buf: nil, meth: 0)
      _map(seq, name: name, opt: opt, buf: buf, meth: meth)
    end

    # Convenience method: map a batch with keyword arguments.
    def map_batch(seqs, names: nil, opt: nil, buf: nil)
      _map_batch(seqs, names: names, opt: opt, buf: buf)
    end
  end
end
