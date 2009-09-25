#! /bin/sh

gcc -g -o capi.bin capi.c -I ../ -L ../ -llmc -lrt && \
    valgrind -v --leak-check=full ./capi.bin 2>.tmp.valgrind

grep "\\.c" .tmp.valgrind
