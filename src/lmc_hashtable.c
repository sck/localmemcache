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

size_t lmc_ht_strdup(void *base, const char *s, size_t l) {
  size_t va_s = lmc_valloc(base, l +  sizeof(size_t) + 1);
  if (!va_s) { return 0; }
  char *p = base + va_s;
  *(size_t *) p = l;
  p += sizeof(size_t);
  memcpy(p, s, l + 1);
  return va_s;
}

unsigned long ht_hash_key(const char *s, size_t l) {
  unsigned long v;
  size_t i;
  for (v = 0, i = 0; i++ < l; s++) { v = *s + 31 * v; }
  return v % LMC_HT_BUCKETS;
}

ht_hash_entry_t lmc_null_node = { 0, 0, 0 };

va_ht_hash_t ht_hash_create(void *base, lmc_error_t *e) {
  va_ht_hash_t va_ht = lmc_valloc(base, sizeof(ht_hash_t));
  if (!va_ht) {
    LMC_MEMORY_POOL_FULL("ht_hash_create");
    return 0;
  }
  memset(base + va_ht, 0, sizeof(ht_hash_t));
  return va_ht;
}

int ht_hash_destroy(void *base, va_ht_hash_t ht) { 
  // ignore: Hashes are only deleted by deleting the namespace region
  return 1;
}

ht_hash_entry_t *ht_lookup(void *base, va_ht_hash_t va_ht, const char *key, 
    size_t n_key) {
  va_ht_hash_entry_t va_hr;
  ht_hash_entry_t *hr = &lmc_null_node;
  ht_hash_t *ht = base + va_ht;
  size_t i;
  for (va_hr = ht->va_buckets[ht_hash_key(key, n_key)]; 
      va_hr != 0 && hr != NULL; ) {
    hr = va_hr ? base + va_hr : 0;
    if (!hr) goto next;
    char *s = base + hr->va_key;
    size_t l = *(size_t *) s;
    if (l != n_key) goto next;
    s += sizeof(size_t);
    for (i = 0; i < l; i++) {
      if (s[i] != key[i]) goto next;
    }
    return hr;

next:
    va_hr = hr->va_next;
  }
  return &lmc_null_node;
}

ht_hash_entry_t *ht_lookup2(void *base, va_ht_hash_t va_ht, char *k) {
  return ht_lookup(base, va_ht, k + sizeof(size_t), *(size_t *) k);
}

const char *ht_get(void *base, va_ht_hash_t va_ht, const char *key, 
    size_t n_key, size_t *n_value) { 
  size_t va = ht_lookup(base, va_ht, key, n_key)->va_value; 
  char *r = va ? base + va : 0;
  if (!r) return 0;
  *n_value = *(size_t *) r;
  return r + sizeof(size_t);
}

size_t lmc_string_len(char *s) { return *(size_t *) s; }
char *lmc_string_data(char *s) { return s + sizeof(size_t); }

int ht_redo(void *base, va_ht_hash_t va_ht, lmc_log_descriptor_t *l, 
    lmc_error_t *e) {
  if (l->op_id == LMC_OP_HT_SET) {
    lmc_log_ht_set *l_set = (lmc_log_ht_set *)l;
    if (l_set->va_key == 0 || l_set->va_value == 0) { return 1; }
    char *k = base + l_set->va_key;
    char *v = base + l_set->va_value;
    ht_set(base, va_ht, lmc_string_data(k), lmc_string_len(k),
        lmc_string_data(v), lmc_string_len(v), e);
    return 1;
  }
  return 0;
}

int ht_set(void *base, va_ht_hash_t va_ht, const char *key, 
    size_t n_key, const char *value, size_t n_value, lmc_error_t *e) {
  ht_hash_t *ht = base + va_ht;
  ht_hash_entry_t *hr = ht_lookup(base, va_ht, key, n_key);
  unsigned v;
  if (hr->va_key == 0) {
    int free_key = 1;
    lmc_log_ht_set *l = (lmc_log_ht_set *)lmc_log_op(base, LMC_OP_HT_SET);
    if ((l->va_value = lmc_ht_strdup(base, value, n_value)) == 0 ||
        (l->va_key = lmc_ht_strdup(base, key, n_key)) == 0) {
      LMC_MEMORY_POOL_FULL("ht_set");
      goto failed;
    }
    free_key = 0;
    va_ht_hash_entry_t va = lmc_valloc(base, sizeof(ht_hash_entry_t));
    hr = va ? base + va : 0;
    if (hr == NULL) { 
      LMC_MEMORY_POOL_FULL("ht_set");
      goto failed;
    }
    LMC_TEST_CRASH
    hr->va_key = l->va_key;
    v = ht_hash_key(key, n_key);
    LMC_TEST_CRASH
    hr->va_next = ht->va_buckets[v];
    ht->va_buckets[v] = va;
    hr->va_value = l->va_value;
    if (free_key) {
      size_t va = l->va_key;
      l->op_id = 0;
      LMC_TEST_CRASH
      lmc_free(base, va);
    }
    lmc_log_finish(base);
  } else {
    LMC_TEST_CRASH
    size_t va = hr->va_value;
    if ((hr->va_value = lmc_ht_strdup(base, value, n_value)) == 0) {
      LMC_MEMORY_POOL_FULL("ht_set");
      goto failed_no_log;
    }
    lmc_free(base, va);
  }
  return 1;

failed:
  lmc_log_finish(base);
failed_no_log:
  return 0;
}

int ht_delete(void *base, va_ht_hash_t va_ht, const char *key, size_t n_key) {
  va_ht_hash_entry_t va_hr;
  ht_hash_entry_t *hr = &lmc_null_node;
  size_t va_p = 0;
  ht_hash_t *ht = base + va_ht;
  size_t i;
  unsigned long k = ht_hash_key(key, n_key);
  for (va_hr = ht->va_buckets[k]; va_hr != 0 && hr != NULL; 
      va_hr = hr->va_next) {
    hr = va_hr ? base + va_hr : 0;
    if (!hr) goto next;
    char *s = base + hr->va_key;
    size_t l = *(size_t *) s;
    if (l != n_key) goto next;
    s += sizeof(size_t);
    for (i = 0; i < l; i++) {
      if (s[i] != key[i]) goto next;
    }

    ht_hash_entry_t *p = va_p ? base + va_p : 0;
    if (p) { p->va_next = hr->va_next; }
    else { ht->va_buckets[k] = 0; }
    lmc_free(base, hr->va_key);
    lmc_free(base, hr->va_value);
    lmc_free(base, va_hr);
    return 1; 

  next:
    va_p = va_hr;
  }
  return 0;
}

int ht_hash_iterate(void *base, va_ht_hash_t va_ht, void *ctx, 
    LMC_ITERATOR_P(iter)) {
  va_ht_hash_entry_t va_hr;
  ht_hash_entry_t *hr = &lmc_null_node;
  ht_hash_t *ht = base + va_ht;
  size_t k;
  for (k = 0; k < LMC_HT_BUCKETS; k++) {
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
  ht_hash_entry_t *hr = &lmc_null_node;
  ht_hash_t *ht = base + va_ht;
  if (!lmc_um_mark_allocated(base, bf, va_ht)) goto failed;
  size_t k;
  for (k = 0; k < LMC_HT_BUCKETS; k++) {
    for (va_hr = ht->va_buckets[k]; va_hr != 0 && hr != NULL; 
        va_hr = hr->va_next) {
      hr = va_hr ? base + va_hr : 0;
      if (!hr) goto next_bucket;
      if (!(lmc_um_mark_allocated(base, bf, va_hr) &&
          lmc_um_mark_allocated(base, bf, hr->va_key) &&
          lmc_um_mark_allocated(base, bf, hr->va_value))) goto failed;
    }
  next_bucket:
    continue;
  }
  lmc_um_find_leaks(base, bf);
  free(bf);
  return 1;
failed:
  free(bf);
  return 0;
}
