require 'rblocalmemcache'

# == Overview
# TestRDocUsage: A useless file
#
# == Example
#
#  Usage:  ruby testRDocUsage.rb  [options]
#
class LocalMemCache

  class LocalMemCacheError < StandardError; end
  class ShmError < LocalMemCacheError; end
  class MemoryPoolFull < LocalMemCacheError; end
  class LockError < LocalMemCacheError; end
  class LockTimedOut < LocalMemCacheError; end
  class OutOfMemoryError < LocalMemCacheError; end
  class RecoveryFailed < LocalMemCacheError; end
  class ShmLockFailed < LocalMemCacheError; end
  class ShmUnlockFailed < LocalMemCacheError; end

  #  Creates a new handle for accessing a shared memory region.
  #
  #  LocalMemCache.new :namespace=>"foo", :size_mb=> 1
  #
  #  The namespace parameter is mandatory.  
  #  The size_mb defaults to 1024 (1 GB).
  #
  def self.new(options)
    o = { :size_mb => 0 }.update(options || {})
    raise "Missing mandatory option ':namespace'" if !o[:namespace]
    _new(o[:namespace].to_s, o[:size_mb].to_f);
  end

  # Deletes the given namespaces, removing semaphores if necessary.
  # Do only use if you are sure the namespace is not used anymore by other
  # processes.
  #
  def self.clear_namespace(namespace, repair = false) 
    _clear_namespace(namespace.to_s, repair)
  end
end
