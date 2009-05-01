$DIR=File.dirname(__FILE__)
['.', '..', '../ruby-binding/'].each {|p| $:.unshift File.join($DIR, p) }

require 'bacon'
require 'localmemcache'

Bacon.summary_on_exit

LocalMemCache.drop :namespace => "speed-comparison", :force => true
$lm2 = LocalMemCache.new :namespace=>"speed-comparison"


10000000.times {
  $lm2.keys
}

#$stdout.write "ht shm setting x 20000: "
#tmeasure (2_000_000) { 
#  v = $lm2.get("f").to_i + 1
#  #puts "v:#{v}"
#  $lm2.set("f", v) 
#}
#puts "foo: #{$lm2.get("f")}"
 
