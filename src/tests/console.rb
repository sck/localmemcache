$DIR=File.dirname(__FILE__)
['.', '..', '../ruby-binding/'].each {|p| $:.unshift File.join($DIR, p) }

require 'localmemcache'
