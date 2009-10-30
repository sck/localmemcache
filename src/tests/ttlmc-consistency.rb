$DIR=File.dirname(__FILE__)
['.', '../', '../ruby-binding/'].each {|p| $:.unshift File.join($DIR, p) }

require 'torture'
require 'localmemcache'

LocalMemCache.drop :namespace => "torture", :force => true
$h = LocalMemCache.new :namespace=>'torture', :size_mb => 200


puts "STARTED: #{$$}"

class LocalMemCache
  def __set(k, v) 
    set(k, v)
    if get(k) != v.to_s
      puts "Set FAILED!"
    end
    raise "Consistency failed" if !check_consistency
  end
  def __delete(k)
    delete(k)
    raise "Consistency failed" if !check_consistency
  end

  def __clear
    clear if rand * 100 > 99
  end

  def __each_pair
    each_pair { } if rand * 100 > 99
  end

  def __keys
    keys if rand * 100 > 99
  end

end


class TortureTesting
  def self.rand_index
    rand(9999)
  end
end

TortureTesting.no_progress

TortureTesting.run(2_000,
  [$h, :get, [:rand_index]],
  [$h, :__set, [:rand_index, :any]],
  [$h, :__delete, [:rand_index]],
  [$h, :__clear],
  [$h, :__keys],
  [$h, :random_pair],
  [$h, :__each_pair]
) {|e| raise e }
