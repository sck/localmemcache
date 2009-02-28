require 'lmctestapi'

def tmeasure(c, &block)
  _then = Time.now
  c.times { block.call }
  now = Time.now
  puts "#{(now - _then)*1000} ms"
end

$lm = SHM.new("foo")

v= nil
ev = nil
1000000.times {|i|
  v = $lm.get(3)
  $lm.set(3, v.to_i + 1)
  ev = v.to_i + 1
}

puts "#{$$} v: #{v}"
