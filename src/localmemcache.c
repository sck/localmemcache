#include "localmemcache.h"
#include <stdio.h>
#include <string.h>
#include "valloc.h"
#include "shm.h"

int lmc_set_lock_flag(void *base, lmc_error_t *e) {
  mem_descriptor_t *md = base;
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
  mem_descriptor_t *md = base;
  if (md->locked != 1) {
    strncpy(e->error_str, "Shared memory region appears to be unlocked already"
        "--may be corrupt.", 1023); 
    return 0;
  } else {
    md->locked = 0;
  }
  return 1;
}

int local_memcache_clear_namespace(const char *namespace, lmc_error_t *e) {
  return lmc_clean_namespace(namespace, e);
}

local_memcache_t *local_memcache_create(const char *namespace, size_t size,
    lmc_error_t* e) {
  local_memcache_t *lmc = calloc(1, sizeof(local_memcache_t));
  if (!lmc || (lmc->namespace = strdup(namespace)) == NULL) return NULL;
  lmc->size = size;
  if ((lmc->lock = lmc_lock_init(lmc->namespace, 1, e)) == NULL) goto failed;
  if (!lmc_is_lock_working(lmc->lock, e)) {
    strncpy(e->error_str, "Failed to lock shared memory!", 1023); 
    goto failed;
  }
  {
    if (!lmc_lock_obtain("local_memcache_create", lmc->lock, &lmc->error)) 
        goto failed;
    if ((lmc->shm = lmc_shm_create(lmc->namespace, lmc->size, 0, e)) == NULL) 
        goto release_and_fail;
    lmc->base = lmc->shm->base;
    if (is_lmc_already_initialized(lmc->base)) {
      if (!lmc_set_lock_flag(lmc->base, e)) goto release_and_fail;
      mem_descriptor_t *md = lmc->base;
      lmc->va_hash = md->va_hash;
    } else {
      lmc_init_memory(lmc->base, lmc->size);
      mem_descriptor_t *md = lmc->base;
      if ((md->va_hash = ht_hash_create(lmc->base, e)) == 0) 
          goto unlock_and_fail;
      lmc->va_hash = md->va_hash;
    }
    lmc_release_lock_flag(lmc->base, e);
    lmc_lock_release("local_memcache_create", lmc->lock, e);
  }
  return lmc;

unlock_and_fail:
   lmc_release_lock_flag(lmc->base, e);
release_and_fail:
   lmc_lock_release("local_memcache_create", lmc->lock, e);
failed:
  free(lmc);
  return NULL;
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
  return r;
}
