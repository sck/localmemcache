$DIR=File.dirname(__FILE__)
['.', '../', '../ruby-binding/'].each {|p| $:.unshift File.join($DIR, p) }

require 'torture'
require 'localmemcache'

$h = LocalMemCache.new :namespace=>'torture', :size_mb => 200

class TortureTesting
  def self.rand_index
    rand(9999)
  end
end

TortureTesting.no_progress

TortureTesting.run(200_000,
  [$h, :get, [:rand_index]],
  [$h, :set, [:rand_index, :any]],
  [$h, :delete, [:rand_index]]
) 
