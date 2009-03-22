$DIR=File.dirname(__FILE__)
['.', '..', '../ruby-binding/'].each {|p| $:.unshift File.join($DIR, p) }

require 'localmemcache'

LocalMemCache.clear_namespace("crash-t", true);
#exit
#puts "c"
#LocalMemCache.check_namespace("crash-t");


$pid_reader = fork {
  lm = LocalMemCache.new :namespace=>"crash-t"
  c = 0;
  200000.times {
    c += 1
    r = rand(10000).to_s
    lm.set("foo", "20")
    v = lm.get("foo")
    if v != "20"
      puts "ERROR2: #{v.inspect}"
    end
  }
}

$pid_reader2 = fork {
  lm = LocalMemCache.new :namespace=>"crash-t"
  c = 0;
  200000.times {
    c += 1
    r = rand(10000).to_s
    lm.set("foo", "20")
    v = lm.get("foo")
    if v != "20"
      puts "ERROR1: #{v.inspect}"
    end
  }
}

Process.wait $pid_reader2
