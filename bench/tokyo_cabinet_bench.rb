$DIR=File.dirname(__FILE__)
['.'].each {|p| $:.unshift File.join($DIR, p) }
 
require 'tokyocabinet'
require './common.rb'
 
def __test(n)
  $cache = TokyoCabinet::BDB.new
  $cache.open("/tmp/casket.bdb", TokyoCabinet::BDB::OWRITER | 
      TokyoCabinet::BDB::OCREAT)
   
  $cache.clear
  measure_time(n) {
    r = rand(10000).to_s
    $cache[r] = r
    $cache[r]
  }
end
 
__test(2_000_000)
