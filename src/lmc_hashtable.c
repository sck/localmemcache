/*
 * Copyright (c) 2009, Sven C. Koehler
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lmc_hashtable.h"
#include "lmc_common.h"

#define LMC_OP_HT_SET 1

typedef struct {
   int op_id;
   size_t va_key;
   size_t va_value;
} lmc_log_ht_set;

size_t ht_strdup(void *base, const char *s) {
  size_t va_s = lmc_valloc(base, strlen(s) + 1);
  if (!va_s) { return 0; }
  //memcpy(base + va_s, s, strlen(s) + 1);
  strcpy(base + va_s, s);
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
  ht_hash_entry_t *hr ;
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

int ht_redo(void *base, va_ht_hash_t va_ht, lmc_log_descriptor_t *l, 
    lmc_error_t *e) {
  if (l->op_id == LMC_OP_HT_SET) {
    lmc_log_ht_set *l_set = (lmc_log_ht_set *)l;
    printf("log: (%zd) key:%s\n", l_set->va_key, base+ l_set->va_key);
    printf("log: (%zd) value:%s\n", l_set->va_value, base + l_set->va_value);
    if (l_set->va_key == 0 || l_set->va_value == 0) {
      printf("OP:set incomplete\n");
      return 1;
    }
    ht_set(base, va_ht, base + l_set->va_key, base + l_set->va_value, e);
    return 1;
  }
  return 0;
}

int ht_set(void *base, va_ht_hash_t va_ht, const char *key, 
    const char *value, lmc_error_t *e) {
  lmc_log_ht_set *l = (lmc_log_ht_set *)lmc_log_op(base, LMC_OP_HT_SET);
  if ((l->va_value = ht_strdup(base, value)) == 0 ||
      (l->va_key = ht_strdup(base, key)) == 0) {
    lmc_handle_error_with_err_string("ht_set", "memory pool full", e);
    return 0; 
  }
  ht_hash_t *ht = base + va_ht;
  ht_hash_entry_t *hr = ht_lookup(base, va_ht, base + l->va_key);
  int free_key = 1;
  unsigned v;
  if (hr->va_key == 0) {
    free_key = 0;
    va_ht_hash_entry_t va = lmc_valloc(base, sizeof(ht_hash_entry_t));
    hr = va ? base + va : 0;
    if (hr == NULL) { 
      lmc_handle_error_with_err_string("ht_set", "memory pool full", e);
      return 0; 
    }
    LMC_TEST_CRASH
    hr->va_key = l->va_key;
    v = ht_hash_key(key);
    LMC_TEST_CRASH
    hr->va_next = ht->va_buckets[v];
    ht->va_buckets[v] = va;
  } else {
    LMC_TEST_CRASH
    lmc_free(base, hr->va_value);
  }
  hr->va_value = l->va_value;
  if (free_key) {
    size_t va = l->va_key;
    l->op_id = 0;
    LMC_TEST_CRASH
    lmc_free(base, va);
  }
  lmc_log_finish(base);
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
      if (p) { p->va_next = hr->va_next; }
      else { ht->va_buckets[k] = 0; }
      lmc_free(base, hr->va_key);
      lmc_free(base, hr->va_value);
      lmc_free(base, va_hr);
      return 1; 
    }
    p = base + hr->va_key;
  }
  return 0;
}

int ht_hash_iterate(void *base, va_ht_hash_t va_ht, void *ctx, ITERATOR_P(iter)) {
  va_ht_hash_entry_t va_hr;
  ht_hash_entry_t *hr;
  ht_hash_t *ht = base + va_ht;
  size_t k;
  for (k = 0; k < HT_BUCKETS; k++) {
    for (va_hr = ht->va_buckets[k]; va_hr != 0 && hr != NULL; 
        va_hr = hr->va_next) {
      hr = va_hr ? base + va_hr : 0;
      iter(ctx, base + hr->va_key, base + hr->va_value);
    }
  }
  return 1;
}

int ht_check_memory(void *base, va_ht_hash_t va_ht) {
  char *bf = lmc_um_new_mem_usage_bitmap(base);
  if (!bf) return 0;
  va_ht_hash_entry_t va_hr;
  ht_hash_entry_t *hr;
  ht_hash_t *ht = base + va_ht;
  lmc_um_mark_allocated(base, bf, va_ht);
  size_t k;
  for (k = 0; k < HT_BUCKETS; k++) {
    for (va_hr = ht->va_buckets[k]; va_hr != 0 && hr != NULL; 
        va_hr = hr->va_next) {
      hr = va_hr ? base + va_hr : 0;
      if (!hr) continue;
      lmc_um_mark_allocated(base, bf, va_hr);
      lmc_um_mark_allocated(base, bf, hr->va_key);
      lmc_um_mark_allocated(base, bf, hr->va_value);
    }
  }
  lmc_um_find_leaks(base, bf);
  free(bf);
  return 1;
}
