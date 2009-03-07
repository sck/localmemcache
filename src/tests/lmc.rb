$DIR=File.dirname(__FILE__)
['.', '..', '../ruby-binding/'].each {|p| $:.unshift File.join($DIR, p) }

require 'bacon'
require 'localmemcache'

Bacon.summary_on_exit

$lm = LocalMemCache.new :namespace=>"testing"

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
    p $lm.keys()
  end

  it 'should support clearing of namespaces' do
    LocalMemCache.clear_namespace("testing");
  end

end

def tmeasure(c, &block)
  _then = Time.now
  c.times { block.call }
  now = Time.now
  puts "#{(now - _then)*1000} ms"
end

$lm2 = LocalMemCache.new :namespace=>"speed-comparison"

def compare_speed(n)
  
  puts "LocalMemCache"
  tmeasure(n) {
    r = rand(10000).to_s
#    $lm2.get(r)
    $lm2.set(r, r)
#    nr = $lm2.get(r)
#    if nr != r
#      $stderr.puts "FAILED: #{nr.inspect} != #{r.inspect}"
#    end
  }
  
  puts "builtin"
  $hh = {}
  tmeasure(n) {
    r = rand(10000).to_s
#    $hh[r]
    $hh[r] = r
#    if $hh[r] != r
#      $stderr.puts "FAILED!"
#    end
  }
end

compare_speed(2_000_000)

#$stdout.write "ht shm setting x 20000: "
#tmeasure (2_000_000) { 
#  v = $lm2.get("f").to_i + 1
#  #puts "v:#{v}"
#  $lm2.set("f", v) 
#}
#puts "foo: #{$lm2.get("f")}"
 
