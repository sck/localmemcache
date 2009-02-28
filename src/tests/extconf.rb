require 'mkmf'

dir = File.dirname(__FILE__)

$defs << "-DRUBY_VERSION_CODE=#{RUBY_VERSION.gsub(/\D/, '')}"

$srcs = ['lmctestapi.c']
$objs = ['lmctestapi.o']

$CFLAGS << " -D_REENTRANT -g -I .."
$LDFLAGS << " ../liblmc.a -lpthread -lrt"

dir_config('lmctestapi')
create_makefile('lmctestapi')
