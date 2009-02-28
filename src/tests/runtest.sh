#! /bin/sh
cd `dirname $0`

test -f Makefile || ruby extconf.rb
test -f mctestapi.so || make
make clean 
make
./hashtable

