$DIR=File.dirname(__FILE__)
['.', '../', '../ruby-binding/'].each {|p| $:.unshift File.join($DIR, p) }

require 'torture'
require 'localmemcache'

LocalMemCache.clear_namespace("torture", true);
$h = LocalMemCache.new :namespace=>'torture', :size_mb => 200

class LocalMemCache
  def __set(k, v) 
    set(k, v)
    if get(k) != v.to_s
      puts "Set FAILED!"
      raise "set failed"
    end
  end
  def __delete(k)
    delete(k)
  end

end


class TortureTesting
  def self.rand_index
    rand(9999)
  end
end

TortureTesting.no_progress

TortureTesting.run(20_000_000,
  [$h, :get, [:rand_index]],
  [$h, :__set, [:rand_index, :any]],
  [$h, :__delete, [:rand_index]]
) 
