#include "localmemcache.h"
#include <stdio.h>
#include <string.h>
#include "valloc.h"
#include "shm.h"

local_memcache_t *local_memcache_create(const char *namespace, size_t size,
    lmc_error_t* e) {
  printf("local_memcache_create\n");
  local_memcache_t *lmc = calloc(1, sizeof(local_memcache_t));
  if (!lmc || (lmc->namespace = strdup(namespace)) == NULL) return NULL;
  lmc->size = size;
  if ((lmc->lock = lmc_lock_init(lmc->namespace, e)) == NULL) goto failed;
  if (!lmc_is_lock_working(lmc->lock, e)) {
    lmc_lock_repair(lmc->lock);
    //
    //strncpy(e->error_str, "Failed to lock shared memory!", 1023); 
    //goto failed;
  }
  {
    if (!lmc_lock_obtain("local_memcache_create", lmc->lock, &lmc->error)) 
        goto failed;
    if ((lmc->shm = lmc_shm_create(lmc->namespace, lmc->size, 0, e)) == NULL) 
        goto release_and_fail;
    lmc->base = lmc->shm->base;
    if (is_lmc_already_initialized(lmc->base)) {
      mem_descriptor_t *md = lmc->base;
      lmc->va_hash = md->va_hash;
    } else {
      lmc_init_memory(lmc->base, lmc->size);
      mem_descriptor_t *md = lmc->base;
      if ((md->va_hash = ht_hash_create(lmc->base, e)) == 0) 
          goto release_and_fail;
      lmc->va_hash = md->va_hash;
    }
    lmc_lock_release("local_memcache_create", lmc->lock, e);
  }
  return lmc;

release_and_fail:
    lmc_lock_release("local_memcache_create", lmc->lock, e);
failed:
  free(lmc);
  return NULL;
}

char *local_memcache_get(local_memcache_t *lmc, const char *key) {
  if (!lmc_lock_obtain("local_memcache_get", lmc->lock, &lmc->error)) return 0;
  char *r = ht_get(lmc->base, lmc->va_hash, key);
  lmc_lock_release("local_memcache_get", lmc->lock, &lmc->error);
  return r;
}

int local_memcache_set(local_memcache_t *lmc, 
   const char *key, const char* value) {
  if (!lmc_lock_obtain("local_memcache_set", lmc->lock, &lmc->error)) return 0;
  int r = ht_set(lmc->base, lmc->va_hash, key, value, &lmc->error);
  lmc_lock_release("local_memcache_set", lmc->lock, &lmc->error);
  return r;
}

int local_memcache_delete(local_memcache_t *lmc, char *key) {
  if (!lmc_lock_obtain("local_memcache_delete", lmc->lock, &lmc->error)) return 0;
  int r = ht_delete(lmc->base, lmc->va_hash, key);
  lmc_lock_release("local_memcache_delete", lmc->lock, &lmc->error);
  return r;
}

int local_memcache_free(local_memcache_t *lmc) {
  lmc_error_t e;
  if (!lmc_lock_obtain("local_memcache_free", lmc->lock, &lmc->error)) return 0;
  int r = ht_hash_destroy(lmc->base, lmc->va_hash);
  lmc_lock_release("local_memcache_free", lmc->lock, &lmc->error);
  lmc_shm_destroy(lmc->shm, &e);
  free(lmc->namespace);
  return r;
}
