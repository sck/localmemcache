$DIR=File.dirname(__FILE__)
['.', '..', '../ruby-binding/'].each {|p| $:.unshift File.join($DIR, p) }

require 'localmemcache'

LocalMemCache.clear_namespace("crash-t", true);


$pids = (1..10).map{ fork {
  lm = LocalMemCache.new :namespace=>"crash-t"
  c = 0;
  200000.times {
    lm.set("boo", "10")
    lm.delete("boo")
    vv = lm.get("boo")
    if vv != "10" && vv != nil
      puts "ERROR while deleting: #{vv.inspect}"
    end
    lm.set("foo", "20")
    v = lm.get("foo")
    if v != "20"
      puts "ERROR2: #{v.inspect}"
    end
  }
}}

Process.wait $pids.last
