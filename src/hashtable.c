/*
 * Copyright (c) 2009, Sven C. Koehler
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hashtable.h"
#include "valloc.h"

size_t ht_strdup(void *base, const char *s) {
  size_t va_s = lmc_valloc(base, strlen(s) + 1);
  if (!va_s) { return 0; }
  memcpy(base + va_s, s, strlen(s) + 1);
  return va_s;
}

unsigned long ht_hash_key(const char *s) {
  unsigned long v;
  for (v = 0; *s != '\0'; s++) { v = *s + 31 * v; }
  return v % HT_BUCKETS;
}

ht_hash_entry_t null_node = { 0, 0, 0 };

va_ht_hash_t ht_hash_create(void *base, lmc_error_t *e) {
  va_ht_hash_t va_ht = lmc_valloc(base, sizeof(ht_hash_t));
  if (!va_ht) {
    lmc_handle_error_with_err_string("ht_hash_create", "memory pool full", e);
    return 0;
  }
  memset(base + va_ht, 0, sizeof(ht_hash_t));
  return va_ht;
}

int ht_hash_destroy(void *base, va_ht_hash_t ht) { 
  // ignore: Hashes are only deleted by deleting the namespace region
  return 1;
}

ht_hash_entry_t *ht_lookup(void *base, va_ht_hash_t va_ht, const char *key) {
  va_ht_hash_entry_t va_hr;
  ht_hash_entry_t *hr;
  ht_hash_t *ht = base + va_ht;
  for (va_hr = ht->va_buckets[ht_hash_key(key)]; 
      va_hr != 0 && hr != NULL; va_hr = hr->va_next) {
    hr = va_hr ? base + va_hr : 0;
    if (hr && (strcmp(key, base + hr->va_key) == 0)) { return hr; }
  }
  return &null_node;
}

char *ht_get(void *base, va_ht_hash_t va_ht, const char *key) { 
  size_t va = ht_lookup(base, va_ht, key)->va_value; 
  char *r = va ? base + va : 0;
  return r;
}

int ht_set(void *base, va_ht_hash_t va_ht, const char *key, 
    const char *value, lmc_error_t *e) {
  ht_hash_t *ht = base + va_ht;
  ht_hash_entry_t *hr = ht_lookup(base, va_ht, key);
  unsigned v;
  if (hr->va_key == 0) {
    va_ht_hash_entry_t va = lmc_valloc(base, sizeof(ht_hash_entry_t));
    hr = va ? base + va : 0;
    if (hr == NULL || (hr->va_key = ht_strdup(base, key)) == 0) { 
      lmc_handle_error_with_err_string("ht_set", "memory pool full", e);
      return 0; 
    }
    v = ht_hash_key(key);
    hr->va_next = ht->va_buckets[v];
    ht->va_buckets[v] = va;
  } else {
    lmc_free(base, hr->va_value);
  }
  if ((hr->va_value = ht_strdup(base, value)) == 0) { 
    lmc_handle_error_with_err_string("ht_set", "memory pool full", e);
    return 0; 
  }
  return 1;
}

int ht_delete(void *base, va_ht_hash_t va_ht, const char *key) {
  va_ht_hash_entry_t va_hr;
  ht_hash_entry_t *hr;
  ht_hash_entry_t *p = NULL;
  ht_hash_t *ht = base + va_ht;
  unsigned long k = ht_hash_key(key);
  for (va_hr = ht->va_buckets[k]; va_hr != 0 && hr != NULL; 
      va_hr = hr->va_next) {
    hr = va_hr ? base + va_hr : 0;
    if (hr && (strcmp(key, base + hr->va_key) == 0)) { 
      // remove previous entry
      if (p) { p->va_next = hr->va_next; }
      else { ht->va_buckets[k] = 0; }
      return 1; 
    }
    p = base + hr->va_key;
  }
  return 0;
}
