# frozen_string_literal: true

# Builds the binding together with the minibwa sources from the submodule.
#
# There is no shared libminibwa to link against, so the C library is compiled
# into the extension. Upstream's Makefile is the reference for which files and
# flags are needed; api-test/Makefile shows that the public mapping API needs
# only libminibwa.a (the LOBJS group) plus -lz -lm.

require 'mkmf'

upstream = File.join(__dir__, 'minibwa')

unless File.exist?(File.join(upstream, 'minibwa.h'))
  abort 'minibwa sources not found. Run: git submodule update --init --recursive'
end

# --- source selection ------------------------------------------------------

# The LOBJS group of upstream's Makefile: everything behind the public API.
core = %w[
  kommon kalloc bwt l2bit options seed map-algo lchain align pe cs format
]

# SSE alignment kernels. On ARM these fall back to s2n-lite.h (NEON) on their
# own, so no extra flag is needed there.
ksw2 = %w[ksw2_extz2_sse ksw2_extd2_sse ksw2_ll_sse]

# Suffix array construction, needed only by Index.build.
sais = %w[libsais libsais64]

# Deliberately NOT compiled:
#
#   index.c       mb_index_build.c #includes it to reach the static
#                 mb_bwt_libsais(); compiling it too would define every
#                 symbol in it twice.
#   bwtgen.c      GPL, and the low-memory BWT construction they implement is
#   QSufSort.c    not offered by this gem. Keeping them out keeps it MIT.
#   mimalloc/     replaces malloc process-wide, which is not something a Ruby
#                 extension may do to its host. See HAVE_KALLOC below.
#   main.c        the command line tool.
#   fastmap.c
#   map-main.c
#   bseq.c        FASTX reading and thread pool for the CLI; reading sequences
#   kthread.c     is the caller's job in Ruby.

binding_srcs = %w[minibwa mb_options mb_index mb_index_build mb_buffer mb_hit]

# Upstream uses the POSIX mmap API to load indexes. Keep the vendored sources
# untouched and provide that small API surface locally when building with
# MinGW, where <sys/mman.h> is not available.
if /mingw|mswin/i.match?(RUBY_PLATFORM)
  binding_srcs << 'win_mman'
  $INCFLAGS << ' -I$(srcdir)/compat'
end

$srcs = binding_srcs.map { |f| "#{f}.c" } +
        (core + ksw2 + sais).map { |f| "minibwa/#{f}.c" }
$objs = $srcs.map { |f| "#{File.basename(f, '.c')}.o" }
$VPATH << '$(srcdir)/minibwa'

# So that our own sources can say #include "minibwa/minibwa.h".
$INCFLAGS << ' -I$(srcdir)'

# --- libraries -------------------------------------------------------------

# l2bit.c reads FASTA through kseq/gzip.
abort 'zlib is required' unless have_header('zlib.h') && have_library('z', 'gzopen')
have_library('m', 'log')

# --- flags -----------------------------------------------------------------

# Required, and quietly consequential: without it mb_opt_init() sets
# MB_F_NO_KALLOC and every allocation goes to the system allocator. Upstream
# defines it whenever mimalloc is not linked, which for us is always. The
# build succeeds either way, so a missing -DHAVE_KALLOC shows up only as lost
# performance.
$defs << '-DHAVE_KALLOC'

# Keep optional CPU instructions local to the alignment kernels so the rest
# of a precompiled extension remains compatible with baseline x86_64 CPUs.
sse_cflags = if RbConfig::CONFIG['host_cpu'].match?(/x86_64|x64|amd64/i)
               %w[-msse4.2 -mpopcnt].select { |flag| try_cflags(flag) }
             else
               []
             end

# libsais parallelises index construction with OpenMP; Options#n_thread has no
# effect on Index.build without it. Everything else is single-threaded here,
# so this is optional. Disable with --disable-openmp.
if enable_config('openmp', true) && try_compile("#include <omp.h>\nint main(void){return 0;}", '-fopenmp')
  $defs << '-DLIBSAIS_OPENMP'
  append_cflags('-fopenmp')
  append_ldflags('-fopenmp')
end

# Hide upstream's symbols. ksw2 and kalloc names are shared with minimap2 and
# other lh3 libraries, which may be loaded into the same process by another
# gem.
append_cflags('-fvisibility=hidden')

create_makefile('minibwa/minibwa')

unless sse_cflags.empty?
  File.open('Makefile', 'a') do |makefile|
    ksw2.each do |source|
      makefile.puts
      makefile.puts "#{source}.o: $(srcdir)/minibwa/#{source}.c"
      makefile.puts "\t$(ECHO) compiling $<"
      makefile.puts "\t$(Q) $(CC) $(INCFLAGS) $(CPPFLAGS) $(CFLAGS) #{sse_cflags.join(' ')} $(COUTFLAG)$@ -c $(CSRCFLAG)$<"
    end
  end
end
