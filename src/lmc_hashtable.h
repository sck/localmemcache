/*
 * Copyright (c) 2009, Sven C. Koehler
 */

#ifndef _LMC_HASHTABLE_H_INCLUDED_
#define _LMC_HASHTABLE_H_INCLUDED_
#include "lmc_error.h"
#include "lmc_valloc.h"

typedef size_t va_string_t;
typedef size_t va_ht_hash_entry_t;

typedef struct {
  va_ht_hash_entry_t va_next;
  va_string_t va_key;
  va_string_t va_value;
} ht_hash_entry_t;

#define LMC_HT_BUCKETS 20731
#define LMC_ITERATOR_P(n) int ((n)) \
    (void *ctx, const char *key, const char *value)

typedef size_t va_ht_hash_t;
typedef struct {
  size_t size;
  va_ht_hash_entry_t va_buckets[LMC_HT_BUCKETS];
} ht_hash_t;

va_ht_hash_t ht_hash_create(void *base, lmc_error_t *e);
int ht_set(void *base, va_ht_hash_t va_ht, const char *key, 
    size_t n_key, const char *value, size_t n_value, lmc_error_t *e);
ht_hash_entry_t *ht_lookup(void *base, va_ht_hash_t va_ht, const char *key, 
    size_t n_key);
const char *ht_get(void *base, va_ht_hash_t va_ht, const char *key, size_t n_key,
    size_t *n_value); 
int ht_delete(void *base, va_ht_hash_t va_ht, const char *key, size_t n_key);
int ht_hash_destroy(void *base, va_ht_hash_t ht);
int ht_hash_iterate(void *base, va_ht_hash_t ht, void *ctx, size_t *ofs,
    LMC_ITERATOR_P(iter));
int ht_random_pair(void *base, va_ht_hash_t va_ht, char **r_key, 
    size_t *n_key, char **r_value, size_t *n_value);

int ht_check_memory(void *base, va_ht_hash_t va_ht);
int ht_redo(void *base, va_ht_hash_t va_ht, lmc_log_descriptor_t *l, 
    lmc_error_t *e);
#endif
