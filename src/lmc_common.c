/*
 * Copyright (c) 2009, Sven C. Koehler
 */

#include "localmemcache.h"
#include <stdio.h>
#include <stdlib.h>

int lmc_test_crash_enabled = 0;

void lmc_test_crash(const char* file, int line, const char *function) {
  if (!lmc_test_crash_enabled) { return; }
  int r = rand();
  if ((r & 63) == 0) {
    printf("[%s:%d %s] CRASHING: %d\n", file, line, function, r);
    char *p = 0x0;
    *p = 0x0;
  }
}

#undef lmc_valloc

size_t lmc_test_valloc_fail(const char *file, int line, const char *function,
    void *base, size_t s) {
  int r = rand();
  if ((r & 255) == 0) {
    printf("[%s:%d %s] valloc fail: %d\n", file, line, function, s);
    return 0;
  }
  return lmc_valloc(base, s);
}

