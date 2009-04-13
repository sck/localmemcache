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
  def self.new(options)
    o = { :size_mb => 0 }.update(options || {})
    _new(o);
  end

  # NOTE: This method is deprecated, use LocalMemCache.clear(*args) instead.
  #
  # Deletes a memory pool.  If the repair flag is set, locked semaphores are
  # removed as well.
  #
  # WARNING: Do only call this method with the repair=true flag if you are sure
  # that you really want to remove this memory pool and no more processes are
  # still using it.
  def self.clear_namespace(namespace, repair = false) 
    clear :namespace => namespace.to_s, :repair => repair
  end

  # NOTE: This method is deprecated, use LocalMemCache.check(*args) instead.
  #
  # Tries to repair a corrupt namespace.  Usually one doesn't call this method
  # directly, it's invoked automatically when operations time out.
  # 
  # valid options are 
  # [:namespace] 
  # [:filename] 
  # 
  # The memory pool must be specified by either setting the :filename or
  # :namespace option.  The default for :repair is false.
  def self.check_namespace(namespace) 
    check :namespace => namespace.to_s
  end
end
