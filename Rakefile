task :default do
end

def manifest
  `git ls-files`.split("\n").select{|n| !%r!/site/!.match(n) }
end

def version() File.read("VERSION").chomp end

desc "Generate a ChangeLog"
task :changelog do
  File.open("ChangeLog", "w") { |out|
    `git log -z`.split("\0").map { |chunk|
      author = chunk[/Author: (.*)/, 1].strip.gsub(/mysites/, "Sven C. Koehler")
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

task :sanity_test do
  sh "./configure && make -C src clean && make -C src && " +
      "(cd src/ruby-binding; ruby extconf.rb) && " +
      "make -C src/ruby-binding/ && " +
      "(cd src/tests; ruby extconf.rb) && " +
      "make -C src/tests/ && ./src/tests/lmc "
end

task :performance_test do
  sh "./configure && make -C src clean && make -C src && " +
      "(cd src/ruby-binding; ruby extconf.rb) && " +
      "make -C src/ruby-binding/ && " +
      "(cd src/tests; ruby extconf.rb) && " +
      "make -C src/tests/ && ./src/tests/lmc; " +
      "./src/tests/bench; " +
      "ls -al /var/tmp/localmemcache/speed-comparison.lmc && " +
      "du -h  /var/tmp/localmemcache/speed-comparison.lmc && " +
      "uname -a && ruby -v "
end

task :c_api_package do
  tgz = "pkg/localmemcache-#{version}.tar.gz"
  sh "test -d pkg || mkdir pkg"
  puts "Creating #{tgz}"
  sh "tar czf #{tgz.inspect} #{manifest.map{|n| n.inspect }.join(" ")}"
end

#task :pushsite => [:rdoc] do
task :pushsite do
  sh "chmod 755 site"
  sh "find site -type d | xargs chmod go+rx"
  sh "find site -type f | xargs chmod go+r"
  sh 'rsync --rsh="ssh -i $HOME/.ssh/id_rsa_oss -l sck" -avz site/ sck@localmemcache.rubyforge.org:/var/www/gforge-projects/localmemcache/'
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
    s.summary         = "A persistent key-value database based on mmap()'ed shared memory."

    s.description = <<-EOF

Localmemcache is a library for C and ruby that aims to provide
an interface similar to memcached but for accessing local data instead of
remote data.  It's based on mmap()'ed shared memory for maximum speed.
Since version 0.3.0 it supports persistence, also making it a fast
alternative to GDBM and Berkeley DB.

    EOF

    s.files = manifest 
    s.extensions = ['configure', 'src/ruby-binding/extconf.rb']
    s.require_path = 'src/ruby-binding'
    s.has_rdoc = true
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

