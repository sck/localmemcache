$DIR=File.dirname(__FILE__)
['.', '..', '../ruby-binding/'].each {|p| $:.unshift File.join($DIR, p) }

require 'bacon'
require 'localmemcache'

Bacon.summary_on_exit

LocalMemCache.drop :namespace => "iteration", :force => true
$l = LocalMemCache.new :namespace=>"iteration"

def compare_speed(n)
  
  puts "lmc: filling dict"
  measure_time(1) { n.times {|i| $l[i] = i } }
  puts "lmc: iterating"
  c = 0
  measure_time(1) {
    $l.each_pair {|k,v| c += 1 }
  }
  raise "lmc: error: c(#{c})!= n(#{n})" if c != n
  
  puts "rhash: filling dict"
  $hh = {}
  measure_time(1) { n.times {|i| $hh[i.to_s] = i.to_s } }
  puts "rhash: iterating"
  c = 0
  measure_time(1) {
    $hh.each {|k,v| c += 1 }
  }
  raise "rhash: error: c!= n" if c != n
end

def measure_time(c, &block)
  _then = Time.now
  c.times { block.call }
  now = Time.now
  puts "#{(now - _then)*1000} ms"
end

compare_speed(400_000)
