$DIR=File.dirname(__FILE__)
['.', '..', '../ruby-binding/'].each {|p| $:.unshift File.join($DIR, p) }

require 'bacon'
require 'localmemcache'

Bacon.summary_on_exit

LocalMemCache.clear_namespace("speed-comparison");
$lm2 = LocalMemCache.new :namespace=>"speed-comparison"

def compare_speed(n)
  
  puts "LocalMemCache"
  measure_time(n) {
    r = rand(10000).to_s
    $lm2.set(r, r)
    $lm2.get(r)
  }
  
  puts "builtin"
  $hh = {}
  measure_time(n) {
    r = rand(10000).to_s
    $hh[r] = r
    $hh[r]
  }
end

def measure_time(c, &block)
  _then = Time.now
  c.times { block.call }
  now = Time.now
  puts "#{(now - _then)*1000} ms"
end

compare_speed(2_000_000)

#$stdout.write "ht shm setting x 20000: "
#tmeasure (2_000_000) { 
#  v = $lm2.get("f").to_i + 1
#  #puts "v:#{v}"
#  $lm2.set("f", v) 
#}
#puts "foo: #{$lm2.get("f")}"
 
