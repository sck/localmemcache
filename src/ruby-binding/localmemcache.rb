require 'rblocalmemcache'

class LocalMemCache
  def self.new(options)
    o = { :size_mb => 0.2 }.update(options || {})
    raise "Missing mandatory option ':namespace'" if !o[:namespace]
    _new(o[:namespace].gsub("/", "-"), (o[:size_mb].to_f * 1024 * 1024).to_i );
  end
end
