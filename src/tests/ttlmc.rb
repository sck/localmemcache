$DIR=File.dirname(__FILE__)
['.', '../', '../ruby-binding/'].each {|p| $:.unshift File.join($DIR, p) }

require 'torture'
require 'localmemcache'

LocalMemCache.clear_namespace("torture", true);
$h = LocalMemCache.new :namespace=>'torture', :size_mb => 200

class LocalMemCache
  def __set(k, v) 
    puts "set: #{k.inspect} => #{v.inspect}"
    set(k, v)
    puts "set done"
    if get(k) != v.to_s
      puts "Set FAILED!"
      raise "set failed"
    end
    puts "se2t done"
    keys();
    puts "se3t done"
  end
  def __delete(k)
    keys()
    puts "delete: #{k.inspect}"
    delete(k)
    keys()
  end

end


class TortureTesting
  def self.rand_index
    rand(9999)
  end
end

TortureTesting.no_progress

TortureTesting.run(200_000,
  [$h, :get, [:rand_index]],
  [$h, :__set, [:rand_index, :any]],
  [$h, :__delete, [:rand_index]]
) 
