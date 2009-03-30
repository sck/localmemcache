$DIR=File.dirname(__FILE__)
['../src/ruby-binding/'].each {|p| $:.unshift File.join($DIR, p) }

require 'localmemcache'
$lm = LocalMemCache.new :namespace => "viewcounters"
$lm[:foo] = 1
$lm[:foo]
$lm.delete(:foo)
