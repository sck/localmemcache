$DIR=File.dirname(__FILE__)
['.', '..', '../ruby-binding/'].each {|p| $:.unshift File.join($DIR, p) }

require 'localmemcache'

LocalMemCache.clear_namespace("alloc-failure-test", true);


  LocalMemCache.enable_test_crash
  $lm2 = LocalMemCache.new :namespace=>"alloc-failure-test"
  2000000.times {
    begin
      r = rand(10000).to_s
      $lm2.set(r, r)
      $lm2.get(r)
    rescue Exception => e
      puts "e: #{e.to_s}"
    end
  }

