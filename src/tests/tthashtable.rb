$DIR=File.dirname(__FILE__)
['.', '../'].each {|p| $:.unshift File.join($DIR, p) }

require 'torture'
require 'lmctestapi'

$h = Hashtable.new

class TortureTesting
  def self.rand_index
    rand(9999)
  end
end

TortureTesting.no_progress

TortureTesting.run(20000,
  [$a, :get, [:rand_index]],
  [$a, :set, [:rand_index, :any]]
) 
