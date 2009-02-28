require 'mkmf'

dir = File.dirname(__FILE__)

$defs << "-DRUBY_VERSION_CODE=#{RUBY_VERSION.gsub(/\D/, '')}"

$srcs = ['rblocalmemcache.c']
$objs = ['rblocalmemcache.o']

$CFLAGS << " -g -I .."
$LDFLAGS << " ../liblmc.a -lpthread -lrt "

dir_config('rblocalmemcache')
create_makefile('rblocalmemcache')
