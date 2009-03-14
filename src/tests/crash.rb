$DIR=File.dirname(__FILE__)
['.', '..', '../ruby-binding/'].each {|p| $:.unshift File.join($DIR, p) }

require 'localmemcache'

#LocalMemCache.clear_namespace("crash-test");
LocalMemCache.check_namespace("crash-test");


begin
$pid = fork {
  LocalMemCache.enable_test_crash
  $lm2 = LocalMemCache.new :namespace=>"crash-test"
  begin 
    r = rand(10000).to_s
    $lm2.set(r, r)
    $lm2.get(r)
  end while true
}

Process.wait $pid
LocalMemCache.disable_test_crash
LocalMemCache.check_namespace("crash-test")
end while true
