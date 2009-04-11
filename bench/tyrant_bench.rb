$DIR=File.dirname(__FILE__)
['.'].each {|p| $:.unshift File.join($DIR, p) }

require 'memcache'
require './common.rb'

def __test(n)
  memcache_ips = ['localhost:2001'] 
  $cache = MemCache.new memcache_ips, 
      :namespace => 'rtn_', 
      :multithreaded => true
  measure_time(n) {
    r = rand(10000).to_s
    $cache[r] = r
    $cache[r]
  }
end

__test(2_000_000)
