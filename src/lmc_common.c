/*
 * Copyright (c) 2009, Sven C. Koehler
 */

#include "localmemcache.h"
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

int lmc_test_crash_enabled = 0;
int lmc_showed_status = 0;

void lmc_test_crash(const char* file, int line, const char *function) {
  if (!lmc_showed_status) {
    lmc_showed_status = 1;
    printf("lmc_test_crash enabled\n");
  }
  if (!lmc_test_crash_enabled) { return; }
  int r = rand();
  if ((r & 63) == 0) {
    printf("[%s:%d %s %d] CRASHING: %d\n", file, line, function, getpid(), r);
    char *p = 0x0;
    *p = 0x0;
  }
}

#undef lmc_valloc

size_t lmc_test_valloc_fail(const char *file, int line, const char *function,
    void *base, size_t s) {
  if (!lmc_showed_status) {
    lmc_showed_status = 1;
    printf("lmc_test_valloc enabled\n");
  }
  int r = rand();
  if ((r & 255) == 0) {
    printf("[%s:%d %s] valloc fail: %zd\n", file, line, function, s);
    return 0;
  }
  return lmc_valloc(base, s);
}

void lmc_clean_string(char *result, const char *source) {
  size_t n = strlen(source);
  if (n > 256) { n = 256; }
  const char *s = source;
  char *d = result;
  char ch;
  for (; n--; d++, s++) {
    ch = *s;
    if ((ch >= 'a' && ch <= 'z') || 
        (ch >= 'A' && ch <= 'Z') ||
	(ch >= '0' && ch <= '9')) {
      *d = ch;
    } else {
      *d = '-';
    }
  }
  *d = 0x0;
}

int lmc_is_filename(const char *s) {
  return strlen(s) > 1 && (s[0] == '/' || (s[0] == '.' && s[1] == '/'));
}
