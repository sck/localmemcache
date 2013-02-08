# -*- encoding: utf-8 -*-

Gem::Specification.new do |s|
  s.name = "localmemcache"
  s.version = "0.4.4"

  s.required_rubygems_version = Gem::Requirement.new(">= 0") if s.respond_to? :required_rubygems_version=
  s.authors = ["Sven C. Koehler"]
  s.date = "2009-11-10"
  s.description = "\nLocalmemcache is a library for C and ruby that aims to provide\nan interface similar to memcached but for accessing local data instead of\nremote data.  It's based on mmap()'ed shared memory for maximum speed.\nSince version 0.3.0 it supports persistence, also making it a fast\nalternative to GDBM and Berkeley DB.\n\n"
  s.email = "schween@snafu.de"
  s.extensions = ["configure", "src/ruby-binding/extconf.rb"]
  s.files = [".gitignore", "AUTHORS", "COPYING", "LICENSE", "Makefile.in", "README", "Rakefile", "THANKS", "VERSION", "aclocal.m4", "bench/common.rb", "bench/gdbm_vs_lmc", "bench/gdbm_vs_lmc.rb", "bench/lmc_bench", "bench/lmc_bench.rb", "bench/memcached_bench", "bench/memcached_bench.rb", "bench/tokyo_cabinet_bench", "bench/tokyo_cabinet_bench.rb", "bench/tyrant_bench", "bench/tyrant_bench.rb", "configure", "configure.in", "example/compile.sh", "example/hello", "example/hello.c", "example/hello.rb", "site/doc/classes/LocalMemCache.html", "site/doc/classes/LocalMemCache.src/M000001.html", "site/doc/classes/LocalMemCache.src/M000002.html", "site/doc/classes/LocalMemCache.src/M000003.html", "site/doc/classes/LocalMemCache.src/M000004.html", "site/doc/classes/LocalMemCache.src/M000005.html", "site/doc/classes/LocalMemCache.src/M000006.html", "site/doc/classes/LocalMemCache.src/M000007.html", "site/doc/classes/LocalMemCache.src/M000008.html", "site/doc/classes/LocalMemCache.src/M000009.html", "site/doc/classes/LocalMemCache.src/M000010.html", "site/doc/classes/LocalMemCache.src/M000011.html", "site/doc/classes/LocalMemCache.src/M000012.html", "site/doc/classes/LocalMemCache.src/M000013.html", "site/doc/classes/LocalMemCache.src/M000014.html", "site/doc/classes/LocalMemCache/ArgError.html", "site/doc/classes/LocalMemCache/DBVersionNotSupported.html", "site/doc/classes/LocalMemCache/InitError.html", "site/doc/classes/LocalMemCache/LocalMemCacheError.html", "site/doc/classes/LocalMemCache/LockError.html", "site/doc/classes/LocalMemCache/LockTimedOut.html", "site/doc/classes/LocalMemCache/MemoryPoolClosed.html", "site/doc/classes/LocalMemCache/MemoryPoolFull.html", "site/doc/classes/LocalMemCache/OutOfMemoryError.html", "site/doc/classes/LocalMemCache/RecoveryFailed.html", "site/doc/classes/LocalMemCache/SharedObjectStorage.html", "site/doc/classes/LocalMemCache/SharedObjectStorage.src/M000015.html", "site/doc/classes/LocalMemCache/SharedObjectStorage.src/M000016.html", "site/doc/classes/LocalMemCache/SharedObjectStorage.src/M000017.html", "site/doc/classes/LocalMemCache/SharedObjectStorage.src/M000018.html", "site/doc/classes/LocalMemCache/ShmError.html", "site/doc/classes/LocalMemCache/ShmLockFailed.html", "site/doc/classes/LocalMemCache/ShmUnlockFailed.html", "site/doc/created.rid", "site/doc/files/extconf_rb.html", "site/doc/files/localmemcache_rb.html", "site/doc/files/rblocalmemcache_c.html", "site/doc/fr_class_index.html", "site/doc/fr_file_index.html", "site/doc/fr_method_index.html", "site/doc/index.html", "site/doc/rdoc-style.css", "site/index.html", "src/Makefile.in", "src/lmc_common.c", "src/lmc_common.h", "src/lmc_config.h.in", "src/lmc_error.c", "src/lmc_error.h", "src/lmc_hashtable.c", "src/lmc_hashtable.h", "src/lmc_lock.c", "src/lmc_lock.h", "src/lmc_shm.c", "src/lmc_shm.h", "src/lmc_valloc.c", "src/lmc_valloc.h", "src/localmemcache.c", "src/localmemcache.h", "src/ruby-binding/extconf.rb", "src/ruby-binding/localmemcache.rb", "src/ruby-binding/rblocalmemcache.c", "src/tests/alloc", "src/tests/alloc.rb", "src/tests/allocfailure", "src/tests/allocfailure.rb", "src/tests/bacon.rb", "src/tests/bench", "src/tests/bench-append", "src/tests/bench-append.rb", "src/tests/bench.rb", "src/tests/bench_keys", "src/tests/bench_keys.rb", "src/tests/capi.c", "src/tests/capi.sh", "src/tests/console", "src/tests/console.rb", "src/tests/crash", "src/tests/crash-small", "src/tests/crash-small.rb", "src/tests/crash.rb", "src/tests/extconf.rb", "src/tests/iter", "src/tests/iter.rb", "src/tests/lmc", "src/tests/lmc.rb", "src/tests/lmctestapi.c", "src/tests/parallelwrite", "src/tests/parallelwrite.rb", "src/tests/run-all-tests", "src/tests/runtest.sh", "src/tests/sanity-test", "src/tests/shm", "src/tests/shm.rb", "src/tests/torture.rb", "src/tests/ttalloc", "src/tests/ttalloc.rb", "src/tests/ttkeys", "src/tests/ttkeys.rb", "src/tests/ttlmc", "src/tests/ttlmc-consistency", "src/tests/ttlmc-consistency.rb", "src/tests/ttlmc.rb", "src/tests/ttrandom_pair", "src/tests/ttrandom_pair.rb"]
  s.homepage = "http://localmemcache.rubyforge.org/"
  s.require_paths = ["src/ruby-binding"]
  s.rubyforge_project = "localmemcache"
  s.rubygems_version = "1.8.23"
  s.summary = "A persistent key-value database based on mmap()'ed shared memory."

  if s.respond_to? :specification_version then
    s.specification_version = 3

    if Gem::Version.new(Gem::VERSION) >= Gem::Version.new('1.2.0') then
    else
    end
  else
  end
end

