$DIR=File.dirname(__FILE__)
['.', '../src/ruby-binding/'].each {|p| $:.unshift File.join($DIR, p) }

require 'common.rb'

def __test(n)
  require 'localmemcache'
  puts "LocalMemCache"
  LocalMemCache.clear :filename => "/tmp/fruitstore.lmc"
  $lm2 = LocalMemCache.new :filename=>"/tmp/fruitstore.lmc"
  measure_time(n) {
    r = rand(10000).to_s
    $lm2.set(r, r)
    $lm2.get(r)
  }

  require 'gdbm'
  puts "GDBM"
  h = GDBM.new("/tmp/fruitstore.db")
  measure_time(n) {
    r = rand(10000).to_s
    h[r] = r
    h[r]
  }
  h.close
end

__test(2_000_000)

