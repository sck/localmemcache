/*
 * Copyright (c) 2009, Sven C. Koehler
 */

#ifndef _LOCAL_MEMCACHE_INCLUDED_
#define _LOCAL_MEMCACHE_INCLUDED_

#include <stdlib.h>
#include "lmc_hashtable.h"
#include "lmc_shm.h"
#include "lmc_lock.h"
#include "lmc_error.h"
#include "lmc_common.h"

#define LOCAL_MEMCACHE_FAILED 0
#define LOCAL_MEMCACHE_SUCCESS 1

typedef struct {
  char *namespace;
  size_t size;
  lmc_shm_t *shm;
  size_t va_hash;
  lmc_lock_t *lock;
  lmc_lock_t *root_lock;
  void* base;
  lmc_error_t error;
} local_memcache_t;

local_memcache_t *local_memcache_create(const char *namespace, 
    const char *filename, double size_mb, lmc_error_t* e);
char *local_memcache_get_new(local_memcache_t *lmc, const char *key, 
    size_t n_key, size_t *n_value);
int local_memcache_set(local_memcache_t *lmc, const char *key, size_t n_key, 
    const char* value, size_t n_value);
int local_memcache_delete(local_memcache_t *lmc, char *key, size_t n_key);
int local_memcache_free(local_memcache_t *lmc, lmc_error_t *e);
int local_memcache_iterate(local_memcache_t *lmc, void *ctx, 
    LMC_ITERATOR_P(iter));
int local_memcache_clear_namespace(const char *namespace, const char *filename,
    int repair, lmc_error_t *e);
int local_memcache_check_namespace(const char *namespace, const char *filename, 
    lmc_error_t *e);

const char *__local_memcache_get(local_memcache_t *lmc, 
    const char *key, size_t n_key, size_t *n_value);

int lmc_unlock_shm_region(const char *who, local_memcache_t *lmc);
#endif
