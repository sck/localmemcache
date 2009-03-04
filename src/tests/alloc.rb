$DIR=File.dirname(__FILE__)
['.', '..'].each {|p| $:.unshift File.join($DIR, p) }

require 'bacon'
require 'lmctestapi'
$a = Alloc.new(1000)

Bacon.summary_on_exit

describe 'Alloc' do

  it 'should support basic allocation and freeing of memory' do
    $a.dispose($a.get(100))
  end

  it 'should merge free chunks' do
    free = $a.free_mem
    v1 = $a.get(100)
    $a.free_mem.should.equal(free - 108)
    v2 = $a.get(100)
    $a.free_mem.should.equal(free - 216)
    $a.free_chunks.should.equal 1
    $a.dispose(v1)
    $a.free_mem.should.equal(free - 108)
    $a.free_chunks.should.equal 2
    $a.dispose(v2)
    $a.free_mem.should.equal free
    $a.free_chunks.should.equal 1
  end

  it 'should handle memory exhaustion' do
    v1 = $a.get($a.free_mem)
    $a.free_mem.should.equal 0
    $a.free_mem.should.equal 0
    $a.free_mem.should.equal 0
    should.raise(OutOfMemoryError) { $a.get(1) }
    $a.dispose(v1)
  end

  it 'should manage free memory only if there is enough mem left to hold the struct ' do
    $a.free_chunks.should.equal 1
    v1 = $a.get($a.free_mem - 100)
    $a.free_chunks.should.equal 1
    v2 = $a.get(90)
    $a.dump
    $a.free_chunks.should.equal 0
    $a.dispose(v1)
    $a.dispose(v2)
  end

  it 'should deal with calling free for already freed items' do
    v = $a.get(90)
    $a.dispose(v)
    $a.dispose(v)
  end

  it 'should deal with calling free with nasty parameters' do
    $a.dispose(0)
  end

end
