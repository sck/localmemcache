$DIR=File.dirname(__FILE__)
['.', '..', '../ruby-binding/'].each {|p| $:.unshift File.join($DIR, p) }

require 'bacon'
require 'localmemcache'

Bacon.summary_on_exit

LocalMemCache.drop :namespace => "speed-comparison", :force => true
$lm2 = LocalMemCache.new :namespace=>"speed-comparison", :min_alloc_size => 1024

def compare_speed(n)
  
  puts "LocalMemCache"
  measure_time(n) {
    r = rand(10000).to_s
    v = $lm2.get(r)
    $lm2.set(r, "#{v}#{r}")
  }
  
  puts "Ruby Hash of Strings"
  $hh = {}
  measure_time(n) {
    r = rand(10000).to_s
    v = $hh[r]
    $hh[r] = "#{v}#{r}"
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

compare_speed(200_000)
#test_gdbm(2_000_000)

#$stdout.write "ht shm setting x 20000: "
#tmeasure (2_000_000) { 
#  v = $lm2.get("f").to_i + 1
#  #puts "v:#{v}"
#  $lm2.set("f", v) 
#}
#puts "foo: #{$lm2.get("f")}"
 
