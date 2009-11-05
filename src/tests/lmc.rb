$DIR=File.dirname(__FILE__)
['.', '..', '../ruby-binding/'].each {|p| $:.unshift File.join($DIR, p) }

require 'bacon'
require 'localmemcache'

Bacon.summary_on_exit

LocalMemCache.drop :namespace => "test", :force => true
$lm = LocalMemCache.new :namespace=>"test", :size_mb => 2

LocalMemCache.drop :namespace => "test-small", :force => true
$lms = LocalMemCache.new :namespace=>"test-small", :size_mb => 1;

describe 'LocalMemCache' do

  it 'should allow to set and query keys' do
    $lm.get("non-existant").should.be.nil
    $lm.set("foo", "1")
    $lm.get("foo").should.equal "1"
  end

  it 'should support has_key?' do
    $lm.has_key?("foo").should.be.true
    $lm.has_key?("non-existant").should.be.false
    $lm[:bar] = nil
    $lm.has_key?(:bar).should.be.true
    $lm.delete(:bar)
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

  it 'should support each_pair' do 
    $lm.each_pair {|k, v| }
  end

  it 'should support \0 in values and keys' do
    $lm["null"] = "foo\0goo"
    $lm["null"].should.equal "foo\0goo"
  end

  it 'should support random_pair' do
    $lm.random_pair.size.should.equal 2
    LocalMemCache.drop :namespace => :empty, :force => true, :size_mb => 2
    ll = LocalMemCache.new :namespace => :empty, :size_mb => 2
    ll.random_pair.should.be.nil
  end

  it 'should be consistent' do
    $lm.check_consistency.should.be.true
  end

  it 'should support iteration' do 
    s = $lm.size
    s.should.equal 3
    c = 0
    $lm.each_pair {|a,b| c += 1 }
    s.should.equal c
  end

  it 'should throw an exception when accessing a closed pool' do
    $lm.close
    should.raise(LocalMemCache::MemoryPoolClosed) { $lm.keys }
  end

  it 'should throw exception if pool is full' do
    $lms["one"] = "a";
    should.raise(LocalMemCache::MemoryPoolFull) { $lms["two"] = "b" * 8000000; }
  end

  it 'should set a minimum size' do
    LocalMemCache.drop :namespace => :toosmall, :force => true
    ll = LocalMemCache.new :namespace => :toosmall, :size_mb => 0.1
    ll.shm_status[:total_bytes].should.equal 1024*1024
  end


  it 'should support clearing of the hashtable' do
    ($lms.size > 0).should.be.true
    $lms.clear
    $lms.size.should.equal 0
  end

  it 'should support size' do
    LocalMemCache.drop :namespace =>"size-test", :force=>true
    ll = LocalMemCache.new :namespace => "size-test", :size_mb => 1
    ll.size.should.equal 0
    ll[:one] = 1
    ll.size.should.equal 1
    ll.delete(:one)
    ll.size.should.equal 0
    ll[:foo] = 1
    ll.size.should.equal 1
    ll[:goo] = 2
    ll.size.should.equal 2
    ll.clear
    ll.size.should.equal 0
  end

  it 'should support checking of namespaces' do 
    LocalMemCache.check :namespace => "test"
  end

  it 'should support dropping of namespaces' do
    LocalMemCache.drop :namespace => "test"
  end

  it 'should support filename parameters' do
    LocalMemCache.drop :filename => ".tmp.a.lmc", :force => true
    lm = LocalMemCache.new :filename => ".tmp.a.lmc", :size_mb => 1
    lm[:boo] = 1
    lm.size.should.equal 1
    File.exists?(".tmp.a.lmc").should.be.true
    LocalMemCache.check :filename => ".tmp.a.lmc"
    LocalMemCache.drop :filename => ".tmp.a.lmc"
  end

end

LocalMemCache.drop :namespace => "test-shared-os", :force => true
$lmsh = LocalMemCache::SharedObjectStorage.new :namespace=>"test-shared-os", 
    :size_mb => 2

describe 'LocalMemCache::SharedObjectStorage' do
  it 'should allow to set and query for ruby objects' do
    $lmsh["non-existant"].should.be.nil
    $lmsh.set("array", [:foo, :boo])
    $lmsh["array"].should.be.kind_of? Array
    $lmsh.get("array").should.be.kind_of? Array
  end

  it 'support iteration' do
    $lmsh.each_pair {|k, v| v.should.be.kind_of? Array }
  end

  it 'support random_pair' do
    $lmsh.random_pair.last.should.be.kind_of? Array
  end

  it 'should support has_key?' do
    $lmsh[:foo] = 1
    $lmsh.has_key?(:foo).should.be.true
    $lmsh.has_key?(:non_existant).should.be.false
    $lmsh[:bar] = nil
    $lmsh.has_key?(:bar).should.be.true
  end
end

LocalMemCache.drop :namespace => "test-expiry", :force => true
$lmex = LocalMemCache::ExpiryCache.new :namespace=>"test-expiry", 
    :size_mb => 2, :interval_secs => 0, :check_interval => 2

describe 'LocalMemCache::ExpiryCache' do
  it 'should expire automatically' do
    $lmex["foo"] = 1
    $lmex["foo"].should.equal 1
    $lmex["foo"].should.be.nil
  end
  it 'should support has_key?' do
    $lmex.has_key?("foo").should.be.false
    $lmex[:bar] = nil
    $lmex.has_key?(:bar).should.be.true
  end
end
