$DIR=File.dirname(__FILE__)
['..', '../ruby-binding/'].each {|p| $:.unshift File.join($DIR, p) }

require 'bacon'
require 'localmemcache'

Bacon.summary_on_exit

$lm = LocalMemCache.new :namespace=>"baba", :size_mb => 1

describe 'LocalMemCache' do

  it 'should allow to set and query keys' do
    $lm.set("foo", "1")
    $lm.get("foo").should.equal "1"
  end

end

def tmeasure(c, &block)
  _then = Time.now
  c.times { block.call }
  now = Time.now
  puts "#{(now - _then)*1000} ms"
end

def compare_speed(n)
  
  puts "Hashtable"
  tmeasure(n) {
    r = rand(10000).to_s
    $lm.get(r)
    $lm.set(r, r)
    nr = $lm.get(r)
    if nr != r
      $stderr.puts "FAILED: #{nr.inspect} != #{r.inspect}"
    end
  }
  
  puts "builtin"
  $hh = {}
  tmeasure(n) {
    r = rand(10000).to_s
    $hh[r]
    $hh[r] = r
    if $hh[r] != r
      $stderr.puts "FAILED!"
    end
  }
end

#compare_speed(20000)

$stdout.write "ht shm setting x 20000: "
#ht.set("f", 1) 
tmeasure (2_000_000) { 
  v = $lm.get("f").to_i + 1
  #puts "v:#{v}"
  $lm.set("f", v) 
}
puts "foo: #{$lm.get("f")}"
 
