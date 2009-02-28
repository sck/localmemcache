/*
 * Copyright (c) 2009, Sven C. Koehler
 */

#ifndef _LMC_HASHTABLE_H_INCLUDED_
#define _LMC_HASHTABLE_H_INCLUDED_
#include "lmc_error.h"

typedef size_t va_string_t;
typedef size_t va_ht_hash_entry_t;

typedef struct {
  va_ht_hash_entry_t va_next;
  va_string_t va_key;
  va_string_t va_value;
} ht_hash_entry_t;

#define HT_BUCKETS 499

typedef size_t va_ht_hash_t;
typedef struct {
  va_ht_hash_entry_t va_buckets[HT_BUCKETS];
} ht_hash_t;


va_ht_hash_t ht_hash_create(void *base, lmc_error_t *e);
int ht_set(void *base, va_ht_hash_t va_ht, const char *key, const char *value,
    lmc_error_t* e);
ht_hash_entry_t *ht_lookup(void *base, va_ht_hash_t va_ht, const char *key);
char *ht_get(void *base, va_ht_hash_t ht, const char *key);
int ht_delete(void *base, va_ht_hash_t va_ht, const char *key);
int ht_hash_destroy(void *base, va_ht_hash_t ht);
#endif
