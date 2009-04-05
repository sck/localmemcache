$DIR=File.dirname(__FILE__)
['.', '..', '../ruby-binding/'].each {|p| $:.unshift File.join($DIR, p) }

require 'bacon'
require 'localmemcache'

Bacon.summary_on_exit

LocalMemCache.clear_namespace("test", true)
$lm = LocalMemCache.new :namespace=>"test"

LocalMemCache.clear_namespace("test-small", true)
$lms = LocalMemCache.new :namespace=>"test-small", :size_mb => 0.01;

describe 'LocalMemCache' do

  it 'should allow to set and query keys' do
    $lm.get("non-existant").should.be.nil
    $lm.set("foo", "1")
    $lm.get("foo").should.equal "1"
  end

  it 'should support the [] and []= operators' do
    $lm["boo"] = "2"
    $lm["boo"].should.equal "2"
  end

  it 'should allow deletion of keys' do
    $lm["deleteme"] = "blah"
    $lm["deleteme"].should.not.be.nil
    $lm.delete("deleteme")
    $lm["deleteme"].should.be.nil
    $lm.delete("non-existant")
  end

  it 'should return a list of keys' do
    $lm.keys().size.should.equal 2
  end

  it 'should support \0 in values and keys' do
    $lm["null"] = "foo\0goo"
    $lm["null"].should.equal "foo\0goo"
  end

  it 'should throw an exception when accessing a closed pool' do
    $lm.close
    should.raise(LocalMemCache::MemoryPoolClosed) { $lm.keys }
  end

  it 'should throw exception if pool is full' do
    $lms["one"] = "a";
    should.raise(LocalMemCache::MemoryPoolFull) { $lms["two"] = "b" * 8000; }
  end

  it 'should support checking of namespaces' do 
    LocalMemCache.check_namespace("test")
  end

  it 'should support clearing of namespaces' do
    LocalMemCache.clear_namespace("test")
  end


end

