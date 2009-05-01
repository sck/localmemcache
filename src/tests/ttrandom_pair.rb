$DIR=File.dirname(__FILE__)
['.', '..', '../ruby-binding/'].each {|p| $:.unshift File.join($DIR, p) }

require 'localmemcache'

LocalMemCache.drop :namespace => "random-pair-test", :force => true
$lm2 = LocalMemCache.new :namespace=>"random-pair-test"

$h = {}
counter = 0
4000.times {|i|
  counter += 1
  $h[i.to_s] = 1
  $lm2[i] = i
}

while counter > 0
  k, v = $lm2.random_pair
  if $h[k] 
    $h[k] = nil
    counter -= 1
  end
end
