$DIR=File.dirname(__FILE__)
['.', '../src/ruby-binding/'].each {|p| $:.unshift File.join($DIR, p) }

require 'localmemcache'
require 'common.rb'

LocalMemCache.drop :namespace=>"speed-comparison", :force => true
$lm2 = LocalMemCache.new :namespace=>"speed-comparison"

def compare_speed(n)
  puts "LocalMemCache"
  measure_time(n) {
    r = rand(10000).to_s
    $lm2.set(r, r)
    $lm2.get(r)
  }
  
  puts "Ruby Hash with strings"
  $hh = {}
  measure_time(n) {
    r = rand(10000).to_s
    $hh[r] = r
    $hh[r]
  }
end

compare_speed(2_000_000)
