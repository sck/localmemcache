$DIR=File.dirname(__FILE__)
['.', '../'].each {|p| $:.unshift File.join($DIR, p) }

require 'torture'
require 'lmctestapi'

$a = Alloc.new(200000)

class TortureTesting
  def self.alloc_v
    rand($a.largest_chunk).to_i
  end
end

class Alloc
  def s_get(s)
    @va ||= []
    @va << get(s)
  end

  def dispose_random
    i = rand(@va.size)
    v = @va[i]
    @va[i] = 0
    dispose(v) if v != 0
  end

  def dealloc_all
    @va.each_with_index {|v, i|
      @va[i] = 0
      dispose(v) if v != 0
    }
  end
end

TortureTesting.no_progress


TortureTesting.run(200000,
  [$a, :s_get, [:alloc_v]],
  [$a, :dispose_random]
) {|a, para, e|
}

$a.dealloc_all

#$a.dump
