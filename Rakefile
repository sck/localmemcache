task :default do
end

def manifest
  `git ls-files`.split("\n").select{|n| !%r!/homepage/!.match(n) }
end

def version() File.read("VERSION") end

desc "Generate a ChangeLog"
task :changelog do
  File.open("ChangeLog", "w") { |out|
    `git log -z`.split("\0").map { |chunk|
      author = chunk[/Author: (.*)/, 1].strip
      date = chunk[/Date: (.*)/, 1].strip
      desc, detail = $'.strip.split("\n", 2)
      detail ||= ""
      detail.rstrip!
      out.puts "#{date}  #{author}"
      out.puts "  * #{desc.strip}"
      out.puts detail  unless detail.empty?
      out.puts
    }
  }
end

#task :pushsite => [:rdoc] do
task :pushsite do
  #sh "rsync -avz doc/ chneukirchen@rack.rubyforge.org:/var/www/gforge-projects/rack/doc/"
  sh "rsync -avz site/ chneukirchen@rack.rubyforge.org:/var/www/gforge-projects/rack/"
end 



begin
  require 'rubygems'

  require 'rake'
  require 'rake/clean'
  require 'rake/packagetask'
  require 'rake/gempackagetask'
  require 'fileutils'
rescue LoadError
else

  spec = Gem::Specification.new do |s|
    s.name            = "localmemcache"
    s.version         = version
    s.platform        = Gem::Platform::RUBY
    s.summary         = "Efficiently sharing a hashtable between " +
        "processes on a local Unix machine."

    s.description = <<-EOF

Localmemcache aims to be faster than using memcached locally by using shared
memory, but providing a similar interface.
    EOF

    s.files = manifest 
    s.extensions = ['configure', 'src/ruby-binding/extconf.rb']
    s.require_path = 'src/ruby-binding'
    s.has_rdoc = false
    s.test_files = Dir['src/test/*.rb']

    s.author = 'Sven C. Koehler'
    s.email = 'schween@snafu.de'
    s.homepage = 'http://localmemcache.rubyforge.org/'
    s.rubyforge_project = 'localmemcache'
  end

  Rake::GemPackageTask.new(spec) do |p|
    p.gem_spec = spec
    p.need_tar = false
    p.need_zip = false
  end

end

