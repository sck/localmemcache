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
    lmc_handle_error_with_err_string("lmc_set_lock_flag",
        "Failed to lock shared memory--may be corrupt!", "ShmLockFailed", e);
    return 0;
  } else {
    md->locked = 1;
  }
  return 1;
}

int lmc_release_lock_flag(void *base, lmc_error_t *e) {
  lmc_mem_descriptor_t *md = base;
  if (md->locked != 1) {
    lmc_handle_error_with_err_string("lmc_release_lock_flag",
        "Shared memory appears to be unlocked already--may be corrupt!", 
        "ShmUnlockFailed", e);
    return 0;
  } else {
    md->locked = 0;
  }
  return 1;
}

void lmc_clean_namespace_string(char *result, const char* original) {
  size_t n = strlen(original);
  if (n > 256) { n = 256; }
  const char *s = original;
  char *d = result;
  char ch;
  for (; n--; d++, s++) { 
    ch = *s;
    if ((ch >= 'a' && ch <= 'z') || 
        (ch >= 'A' && ch <= 'Z')) {
      *d = ch;
    } else {
      *d = '-';
    }
  }
  *d = 0x0;
}

int local_memcache_clear_namespace(const char *namespace, int repair, 
    lmc_error_t *e) {
  char clean_ns[1024];
  lmc_clean_namespace_string((char *)clean_ns, namespace);
  lmc_clean_namespace((char *)clean_ns, e);
  printf("cln2\n");
  if (repair) { 
    lmc_lock_t *l = lmc_lock_init((char *)clean_ns, 1, e);
    printf("cln3\n");
    lmc_lock_repair(l);
    printf("cln4\n");
    free(l);
    char check_lock_name[1024];
    snprintf((char *)&check_lock_name, 1023, "%s-check", (char *)clean_ns);
    lmc_lock_t *check_l;
    check_l =  lmc_lock_init(check_lock_name, 1, e);
    lmc_lock_repair(check_l);
    free(check_l);
  }
  return 1;
}

local_memcache_t *__local_memcache_create(const char *namespace, size_t size, 
    int force, int *ok, lmc_error_t* e) {
  int d;
  printf("create\n");
  if (!ok) { ok = &d; }
  *ok = 1;
  local_memcache_t *lmc = calloc(1, sizeof(local_memcache_t));
  if (!lmc || (lmc->namespace = strdup(namespace)) == NULL) return NULL;
  lmc->size = size;
  if ((lmc->lock = lmc_lock_init(lmc->namespace, 1, e)) == NULL) goto failed;
  int retry_counter = 0;
retry:
  if (retry_counter++ > 10) {
    lmc_handle_error_with_err_string("local_memcache_create",
        "Too many retries: Failed to repair shared memory!", 
        "ShmLockFailed", e);
    goto failed;
  }
  printf("lmc_is_lock_working\n");
  if (!lmc_is_lock_working(lmc->lock, e)) {
    if (!force) {
      if (local_memcache_check_namespace(namespace, e))  goto retry;
      lmc_handle_error_with_err_string("local_memcache_create",
          "Failed to repair shared memory!", "ShmLockFailed", e);
      goto failed;
    } 
    *ok = 0;
  }
  printf("l2\n");
  {
    if (*ok && !lmc_lock_obtain("local_memcache_create", lmc->lock, &lmc->error)) 
        goto failed;
    if ((lmc->shm = lmc_shm_create(lmc->namespace, lmc->size, 0, e)) == NULL) 
        goto release_and_fail;
    lmc->base = lmc->shm->base;
    if (!*ok || is_lmc_already_initialized(lmc->base)) {
      if (*ok && !lmc_set_lock_flag(lmc->base, e)) { 
        if (!force)  goto release_and_fail;
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

local_memcache_t *local_memcache_create(const char *namespace, double size_mb, 
    lmc_error_t* e) {  
  char clean_ns[1024];
  double s = size_mb == 0.0 ? 1024.0 : size_mb;
  size_t si = s * 1024 * 1024;
  //printf("size: %f, s: %f, si: %zd\n", size_mb, s, si);
  lmc_clean_namespace_string((char *)clean_ns, namespace);
  return __local_memcache_create((char *)clean_ns, si, 0, 0, e);
}

int local_memcache_check_namespace(const char *namespace, lmc_error_t *e) {
  char clean_ns[1024];
  lmc_clean_namespace_string((char *)clean_ns, namespace);
  char check_lock_name[1024];
  snprintf((char *)check_lock_name, 1023, "%s-check", (char *)clean_ns);

  if (!lmc_does_namespace_exist((char *)clean_ns)) { 
    lmc_clear_namespace_lock(check_lock_name);
    lmc_clear_namespace_lock(namespace);
    printf("namespace '%s' does not exist!\n", (char *)clean_ns);
    return 1;
  }

  lmc_lock_t *check_l;
  if ((check_l = lmc_lock_init(check_lock_name, 1, e)) == NULL)  {
    lmc_handle_error_with_err_string("lmc_lock_init", 
        "Unable to initialize lock for checking namespace", "LockError", e);
    return 0;
  }
  if (!lmc_lock_obtain_mandatory("local_memcache_check_namespace", 
      check_l, e))  goto check_lock_failed;
  lmc_mem_descriptor_t *md = 0;
  size_t ns_size = lmc_namespace_size((char *)clean_ns);
  int ok;
  local_memcache_t *lmc = __local_memcache_create((char *)clean_ns, ns_size, 
      1, &ok, e);
  if (!lmc) {
    printf("WOAH: lmc == 0!\n");
    goto failed;
  }
  md = lmc->base;
  if (!ok) {
    printf("[lmc] Auto repairing namespace '%s'\n", namespace);
    if (!md->locked) goto release;
    if (md->log.op_id == 0) goto unlock_and_release;
    if (ht_redo(lmc->base, md->va_hash, &md->log, e)) goto unlock_and_release;
    goto failed;
  }
  goto release_but_no_lock_correction;

unlock_and_release:
  if (md) {
    if (!ht_check_memory(lmc->base, md->va_hash)) goto failed;
    md->locked = 0;
  }
release:
  {
    int v; 
    sem_getvalue(lmc->lock->sem, &v);
    if (v == 0) {
      lmc_lock_release("local_memcache_create", lmc->lock, e);
    }
  }
release_but_no_lock_correction:
  local_memcache_free(lmc, e);
  lmc_lock_release("local_memcache_check_namespace", check_l, e);
  free(check_l);
  return 1;
failed:
  lmc_handle_error_with_err_string("local_memcache_check_namespace",
      "Unable to recover namespace", "RecoveryFailed", e);
  local_memcache_free(lmc, e);
  lmc_lock_release("local_memcache_check_namespace", check_l, e);
check_lock_failed:
  free(check_l);
  printf("[lmc] Failed to repair namespace '%s'\n", namespace);
  return 0;
}


int lmc_lock_shm_region(const char *who, local_memcache_t *lmc) {
  int r;
  int retry_counter = 0;
retry:
  if (retry_counter++ > 10) {
    printf("[lmc] Too many retries: Cannot repair namespace '%s'\n", 
        lmc->namespace);
    return 0;
  }
  r = lmc_lock_obtain(who, lmc->lock, &lmc->error);
  if (!r && (strcmp(lmc->error.error_type, "LockTimedOut") == 0)) {
    if (local_memcache_check_namespace(lmc->namespace, &lmc->error))  goto retry;
    printf("[lmc] Cannot repair namespace '%s'\n", lmc->namespace);
  }
  if (!r) return 0;
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

const char *__local_memcache_get(local_memcache_t *lmc, 
    const char *key, size_t n_key, size_t *n_value) {
  if (!lmc_lock_shm_region("local_memcache_get", lmc)) return 0;
  const char *r = ht_get(lmc->base, lmc->va_hash, key, n_key, n_value);
  return r;
}

char *local_memcache_get_new(local_memcache_t *lmc, 
    const char *key, size_t n_key, size_t *n_value) {
  const char *r = __local_memcache_get(lmc, key, n_key, n_value);
  char *new_s = malloc(*n_value);
  memcpy(new_s, r, *n_value);
  if (!lmc_unlock_shm_region("local_memcache_get_new", lmc)) return 0;
  return new_s;
}

int local_memcache_set(local_memcache_t *lmc, 
   const char *key, size_t n_key, const char* value, size_t n_value) {
  if (!lmc_lock_shm_region("local_memcache_set", lmc)) return 0;
  int r = ht_set(lmc->base, lmc->va_hash, key, n_key, value, n_value, 
      &lmc->error);
  if (!lmc_unlock_shm_region("local_memcache_set", lmc)) return 0;
  return r;
}

int local_memcache_delete(local_memcache_t *lmc, char *key, size_t n_key) {
  if (!lmc_lock_shm_region("local_memcache_delete", lmc)) return 0;
  int r = ht_delete(lmc->base, lmc->va_hash, key, n_key);
  if (!lmc_unlock_shm_region("local_memcache_delete", lmc)) return 0;
  return r;
}

int local_memcache_free(local_memcache_t *lmc, lmc_error_t *e) {
  if (!lmc_lock_shm_region("local_memcache_free", lmc)) return 0;
  int r = ht_hash_destroy(lmc->base, lmc->va_hash);
  if (!lmc_unlock_shm_region("local_memcache_free", lmc)) return 0;
  lmc_shm_destroy(lmc->shm, e);
  free(lmc->namespace);
  free(lmc->lock);
  return r;
}

int local_memcache_iterate(local_memcache_t *lmc, void *ctx, 
    LMC_ITERATOR_P(iter)) {
  if (!lmc_lock_shm_region("local_memcache_iterate", lmc)) return 0;
  int r = ht_hash_iterate(lmc->base, lmc->va_hash, ctx, iter);
  if (!lmc_unlock_shm_region("local_memcache_iterate", lmc)) return 0;
  return r;
}
