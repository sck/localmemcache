$DIR=File.dirname(__FILE__)
['.', '..', '../ruby-binding/'].each {|p| $:.unshift File.join($DIR, p) }

require 'localmemcache'

LocalMemCache.drop :namespace => "keys-test", :force => true
$lm2 = LocalMemCache.new :namespace=>"keys-test"

def __key_size2(lm)
  c = 0
  lm.each_pair {|k, v| 
    c +=1
    vv = "v#{k}"
    if v != vv
      puts "Failed value: #{v.inspect}, e: #{vv.inspect}"
      exit
    end
  }
  c
end

$c= 0
$diff = 0
4000.times {
  $c += 1
  $lm2[$c] = "v#{$c}"
  if $lm2.keys.size != $c
    puts "WOHAA: #{$diff} #{$lm2.keys.size} != #{$c}"
    exit 2
  end
  if __key_size2($lm2) != $c
    puts "KS2 WOHAA: #{$diff} #{__key_size2($lm2)} != #{$c}"
    exit 2
  end
}
