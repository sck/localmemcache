$DIR=File.dirname(__FILE__)
['.', '..', '../ruby-binding/'].each {|p| $:.unshift File.join($DIR, p) }

require 'bacon'
require 'localmemcache'

Bacon.summary_on_exit

LocalMemCache.drop :namespace => "speed-comparison", :force => true
$lm2 = LocalMemCache.new :namespace=>"speed-comparison"

def compare_speed(n)
  
  puts "LocalMemCache"
  measure_time(n) {
    r = rand(10000).to_s
    $lm2.set(r, r)
    $lm2.get(r)
  }
  
  puts "Ruby Hash of Strings"
  $hh = {}
  measure_time(n) {
    r = rand(10000).to_s
    $hh[r] = r
    $hh[r]
  }
end

def test_gdbm(n)
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


def measure_time(c, &block)
  _then = Time.now
  c.times { block.call }
  now = Time.now
  puts "#{(now - _then)*1000} ms"
end

compare_speed(2_000_000)
