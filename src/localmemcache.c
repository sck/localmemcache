/*
 * Copyright (c) 2009, Sven C. Koehler
 */

#include "localmemcache.h"
#include <stdio.h>
#include <string.h>
#include "lmc_valloc.h"
#include "lmc_shm.h"

int lmc_set_lock_flag(void *base, lmc_error_t *e) {
  lmc_mem_descriptor_t *md = base;
  if (md->locked != 0) {
    strncpy(e->error_str, "Failed to lock shared memory region--"
        "may be corrupt.", 1023); 
    return 0;
  } else {
    md->locked = 1;
  }
  return 1;
}

int lmc_release_lock_flag(void *base, lmc_error_t *e) {
  lmc_mem_descriptor_t *md = base;
  if (md->locked != 1) {
    strncpy(e->error_str, "Shared memory region appears to be unlocked already"
        "--may be corrupt.", 1023); 
    return 0;
  } else {
    md->locked = 0;
  }
  return 1;
}

int local_memcache_clear_namespace(const char *namespace, int repair, 
    lmc_error_t *e) {
  lmc_clean_namespace(namespace, e);
  if (repair) { 
    lmc_lock_t *l = lmc_lock_init(namespace, 1, e);
    lmc_lock_repair(l);
    free(l);
  }
  return 1;
}

local_memcache_t *__local_memcache_create(const char *namespace, size_t size, 
    int force, int *ok, lmc_error_t* e) {
  int d;
  if (!ok) { ok = &d; }
  *ok = 1;
  local_memcache_t *lmc = calloc(1, sizeof(local_memcache_t));
  if (!lmc || (lmc->namespace = strdup(namespace)) == NULL) return NULL;
  lmc->size = size;
  if ((lmc->lock = lmc_lock_init(lmc->namespace, 1, e)) == NULL) goto failed;
  if (!lmc_is_lock_working(lmc->lock, e)) {
    printf("lmc_is_lock_working\n");
    if (!force) {
      strncpy(e->error_str, "Failed to lock shared memory!", 1023); 
      goto failed;
    }
    *ok = 0;
  }
  {
    if (*ok && !lmc_lock_obtain("local_memcache_create", lmc->lock, &lmc->error)) 
        goto failed;
    if ((lmc->shm = lmc_shm_create(lmc->namespace, lmc->size, 0, e)) == NULL) 
        goto release_and_fail;
    lmc->base = lmc->shm->base;
    if (!*ok || is_lmc_already_initialized(lmc->base)) {
      if (*ok && !lmc_set_lock_flag(lmc->base, e)) { 
        if (!force)  goto release_and_fail;
        printf("lmc_set_lock_flag\n");
        *ok = 0;
      }
      lmc_mem_descriptor_t *md = lmc->base;
      lmc->va_hash = md->va_hash;
    } else {
      lmc_init_memory(lmc->base, lmc->size);
      lmc_mem_descriptor_t *md = lmc->base;
      if ((md->va_hash = ht_hash_create(lmc->base, e)) == 0) 
          goto unlock_and_fail;
      lmc->va_hash = md->va_hash;
    }
    if (*ok) { 
      lmc_release_lock_flag(lmc->base, e);
      lmc_lock_release("local_memcache_create", lmc->lock, e); 
    }
  }
  return lmc;

unlock_and_fail:
   lmc_release_lock_flag(lmc->base, e);
release_and_fail:
   lmc_lock_release("local_memcache_create", lmc->lock, e);
failed:
  *ok = 0;
  free(lmc);
  return NULL;
}

local_memcache_t *local_memcache_create(const char *namespace, size_t size, 
    lmc_error_t* e) {  
  return __local_memcache_create(namespace, size, 0, 0, e);
}

int local_memcache_check_namespace(const char *namespace, lmc_error_t *e) {
  if (!lmc_does_namespace_exist(namespace)) { 
    printf("namespace '%s' does not exist!\n", namespace);
    return 1;
  }
  lmc_mem_descriptor_t *md = 0;
  size_t ns_size = lmc_namespace_size(namespace);
  int ok;
  local_memcache_t *lmc = __local_memcache_create(namespace, ns_size, 1, &ok, e);
  printf("lmc: %x\n", lmc);
  if (!lmc) {
    printf("WOAH: lmc == 0!\n");
    return 0;
  }
  md = lmc->base;
  if (!ok) {
    printf("namespace is not OK!\n");
    printf("e: %s\n", e->error_str);
    if (!md->locked) {
      printf("Just has a stale lock\n");
      goto release;
    }
    printf("md: %x\n", md);
    printf("log: %d\n", md->log.op_id);
    if (md->log.op_id == 0) {
      printf("No op_id set..\n");
      goto unlock_and_release;
    }
    if (ht_redo(lmc->base, md->va_hash, &md->log, e)) {
      goto unlock_and_release;
    }
    return 0;
  }
  if (md && !ht_check_memory(lmc->base, md->va_hash)) goto failed;
  return 1;

unlock_and_release:
  if (md) {
    if (!ht_check_memory(lmc->base, md->va_hash)) goto failed;
    printf("check OK\n");
    md->locked = 0;
  }
release:
  lmc_lock_release("local_memcache_create", lmc->lock, e);
  local_memcache_free(lmc);
  return 1;
failed:
  strncpy(e->error_str, "Unable to recover namespace", 1023); 
  lmc_lock_release("local_memcache_create", lmc->lock, e);
  local_memcache_free(lmc);
  return 0;
}


int lmc_lock_shm_region(const char *who, local_memcache_t *lmc) {
  if (!lmc_lock_obtain(who, lmc->lock, &lmc->error)) return 0;
  if (!lmc_set_lock_flag(lmc->base, &lmc->error)) {
    lmc_lock_release(who, lmc->lock, &lmc->error);
    return 0;
  }
  return 1;
}

int lmc_unlock_shm_region(const char *who, local_memcache_t *lmc) {
  int r = 1;
  if (!lmc_release_lock_flag(lmc->base, &lmc->error)) r = 0;
  lmc_lock_release(who, lmc->lock, &lmc->error);
  return r;
}

char *local_memcache_get(local_memcache_t *lmc, const char *key) {
  if (!lmc_lock_shm_region("local_memcache_get", lmc)) return 0;
  char *r = ht_get(lmc->base, lmc->va_hash, key);
  if (!lmc_unlock_shm_region("local_memcache_get", lmc)) return 0;
  return r;
}

int local_memcache_set(local_memcache_t *lmc, 
   const char *key, const char* value) {
  if (!lmc_lock_shm_region("local_memcache_set", lmc)) return 0;
  int r = ht_set(lmc->base, lmc->va_hash, key, value, &lmc->error);
  if (!lmc_unlock_shm_region("local_memcache_get", lmc)) return 0;
  return r;
}

int local_memcache_delete(local_memcache_t *lmc, char *key) {
  if (!lmc_lock_shm_region("local_memcache_delete", lmc)) return 0;
  int r = ht_delete(lmc->base, lmc->va_hash, key);
  if (!lmc_unlock_shm_region("local_memcache_delete", lmc)) return 0;
  return r;
}

int local_memcache_free(local_memcache_t *lmc) {
  lmc_error_t e;
  if (!lmc_lock_shm_region("local_memcache_free", lmc)) return 0;
  int r = ht_hash_destroy(lmc->base, lmc->va_hash);
  if (!lmc_unlock_shm_region("local_memcache_free", lmc)) return 0;
  lmc_shm_destroy(lmc->shm, &e);
  free(lmc->namespace);
  free(lmc->lock);
  return r;
}

int local_memcache_iterate(local_memcache_t *lmc, void *ctx, ITERATOR_P(iter)) {
  if (!lmc_lock_shm_region("local_memcache_iterate", lmc)) return 0;
  int r = ht_hash_iterate(lmc->base, lmc->va_hash, ctx, iter);
  if (!lmc_unlock_shm_region("local_memcache_delete", lmc)) return 0;
  return r;
}

