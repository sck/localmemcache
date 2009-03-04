task :default do
end

def manifest
  r = `git ls-files`.split("\n")
  p r
  r
end

def version() File.read("VERSION") end

puts "version: #{version}"

desc "Generate a ChangeLog"
task :changelog do
  File.open("ChangeLog", "w") { |out|
    `git log -z`.split("\0").map { |chunk|
      author = chunk[/Author: (.*)/, 1].strip
      date = chunk[/Date: (.*)/, 1].strip
      desc, detail = $'.strip.split("\n", 2)
      detail ||= ""
      detail = detail.gsub(/.*darcs-hash:.*/, '')
      detail.rstrip!
      out.puts "#{date}  #{author}"
      out.puts "  * #{desc.strip}"
      out.puts detail  unless detail.empty?
      out.puts
    }
  }
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

    s.files           = manifest 
    s.bindir          = 'bin'
    s.require_path    = 'lib'
    s.has_rdoc        = true
    s.extra_rdoc_files = #['README', 'SPEC', 'RDOX', 'KNOWN-ISSUES']
    s.test_files      = Dir['test/*.rb']

    s.author          = 'Sven C. Koehler'
    s.email           = 'schween@snafu.de'
    #s.homepage        = 'http://localmemcache.rubyforge.org/'
    s.rubyforge_project = 'localmemcache'
  end

  Rake::GemPackageTask.new(spec) do |p|
    p.gem_spec = spec
    p.need_tar = false
    p.need_zip = false
  end

end

