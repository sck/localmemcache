$DIR=File.dirname(__FILE__)
['.', '..', '../ruby-binding/'].each {|p| $:.unshift File.join($DIR, p) }

require 'localmemcache'

LocalMemCache.drop :namespace => "crash-small-t", :force => true
$lm = LocalMemCache.new :namespace=>"crash-small-t"

$lm["one"] = "1"
$lm.check_consistency

