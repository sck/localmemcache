require 'mkmf'

dir = File.dirname(__FILE__)

$defs << "-DRUBY_VERSION_CODE=#{RUBY_VERSION.gsub(/\D/, '')}"

$srcs = ['lmctestapi.c']
$objs = ['lmctestapi.o']

$CFLAGS << "-g -I .."
$LDFLAGS << " -lpthread "
$LOCAL_LIBS << "../liblmc.a"
if have_library("rt")
  $LDFLAGS << " -lrt"
end

dir_config('lmctestapi')
create_makefile('lmctestapi')
