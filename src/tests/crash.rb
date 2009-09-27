$DIR=File.dirname(__FILE__)
['.', '..', '../ruby-binding/'].each {|p| $:.unshift File.join($DIR, p) }

require 'localmemcache'

LocalMemCache.drop :namespace => "crash-t", :force => true

$pids = []
5.times { $pids << fork {
  lm = LocalMemCache.new :namespace=>"crash-t"
  puts "pid: #{$$}"
  c = 0;
  40000000.times {
      c += 1
      r = rand(10000).to_s
      lm.set(r, r)
      lm.get(r) == r
  }
  puts "#{$$} Worker finished"
}}

10.times {
$pid = fork {
  LocalMemCache.enable_test_crash
  $lm2 = LocalMemCache.new :namespace=>"crash-t"
  2000.times {
    r = rand(10000).to_s
    $lm2.set(r, r)
    $lm2.get(r)
  }
}
Process.wait $pid
sleep 3
}

$pids.each {|p| Process.kill "TERM", p }
Process.wait $pids.last
