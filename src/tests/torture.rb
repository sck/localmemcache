# Copyright (C) 2009 Sven C. Koehler <schween@snafu.de>
# torture.rb is freely distributable under the terms of an MIT-style license.
# See http://www.opensource.org/licenses/mit-license.php.

$rand_seed = (ENV['TTSEED'] || Time.now).to_i
puts "torture rand seed: #{$rand_seed}"
srand $rand_seed

$stdout.sync = true
$big = 2**34
class TortureTesting
  class << self
    def string(size=500)
      @cs ||= (0..255).to_a
      (0...size).collect { @cs[Kernel.rand(256)].chr }.join
    end
    def float(max=$big) rand * max end
    def numeric(max=$big) rand(max) end
    def nil() nil end
    def any() self.send([:numeric, :float, :string, :nil][rand(4)]) end

    def run(n=2000, *args, &block) @calls = args; torture(n, &block) end
    def rand_param(k) 
      return self.send(k) if Symbol === k
      if Hash === k
        return k.map {|k,v| [k, rand_param(v)] }.inject({}){|h,p| 
	    k, v = p; h[k] = v; h}
      end
      k 
    end
    def randomize_parameters(d) d.map {|p| rand_param(p) } end
    def no_progress() @no_progress = 1; end

    def torture(n=2000, &handle_exception) 
      puts "Starting torture: #{n}"
      n.times {|c|
        $stdout.write "#{c}\r" if !@no_progress
        d = @calls[rand(@calls.size)]
        d << [] if d.size == 2
        _obj, method, parameter_description = d
	obj = Symbol === _obj ? self.send(obj) : _obj
        begin
          para = randomize_parameters(parameter_description)
          obj.send(*([method] + para))
	rescue Exception => e
	  r = handle_exception.call(obj, para, e) if handle_exception 
	  if r
	    puts "FAILED #{method}: #{para.inspect}"
	    break
	  end
        end
      }
    end
  end
end

