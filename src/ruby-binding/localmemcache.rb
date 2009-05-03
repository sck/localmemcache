require 'rblocalmemcache'

class LocalMemCache

  class LocalMemCacheError < StandardError; end
  class ShmError < LocalMemCacheError; end
  class MemoryPoolFull < LocalMemCacheError; end
  class LockError < LocalMemCacheError; end
  class LockTimedOut < LocalMemCacheError; end
  class OutOfMemoryError < LocalMemCacheError; end
  class ArgError < LocalMemCacheError; end
  class InitError < LocalMemCacheError; end
  class RecoveryFailed < LocalMemCacheError; end
  class ShmLockFailed < LocalMemCacheError; end
  class ShmUnlockFailed < LocalMemCacheError; end
  class MemoryPoolClosed < LocalMemCacheError; end

  #  Creates a new handle for accessing a shared memory region.
  # 
  #  LocalMemCache.new :namespace=>"foo", :size_mb=> 1
  # 
  #  LocalMemCache.new :filename=>"./foo.lmc"
  # 
  #  You must supply at least a :namespace or :filename parameter
  #  The size_mb defaults to 1024 (1 GB).
  #
  #  If you use the :namespace parameter, the .lmc file for your namespace will
  #  reside in /var/tmp/localmemcache.  This can be overriden by setting the
  #  LMC_NAMESPACES_ROOT_PATH variable in the environment.
  #
  #  When you first call .new for a previously not existing memory pool, a
  #  sparse file will be created and memory and disk space will be allocated to
  #  hold the empty hashtable (about 100K), so the size_mb refers
  #  only to the maximum size of the memory pool.  .new for an already existing
  #  memory pool will only map the already previously allocated RAM into the
  #  virtual address space of your process.  
  #
  # 
  def self.new(options)
    o = { :size_mb => 0 }.update(options || {})
    _new(o);
  end

  class SharedObjectStorage < LocalMemCache
    def []=(key,val) super(key, Marshal.dump(val)) end
    def [](key) v = super(key); v.nil? ? nil : Marshal.load(v) end
    def each_pair(&block) 
      super {|k, mv| block.call(k, Marshal.load(mv)) }
    end
    def random_pair
      rp = super
      rp.nil? ? nil : [rp.first, Marshal.load(rp.last)]
    end
  end

end
