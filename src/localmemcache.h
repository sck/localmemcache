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

/*
 * LocalMemCache provides for a Hashtable of strings in shared memory (via a
 * memory mapped file), which thus can be shared between processes on a
 * computer.  Here is an example of its usage:
 *
 * #include <stdio.h>
 * #include <localmemcache.h>
 * 
 * int main() {
 *   lmc_error_t e;
 *   // To use a filename instead of a namespace: 
 *   // lmc = local_memcache_create(0, "filename.lmc", 0, &e);
 *   local_memcache_t *lmc = local_memcache_create("viewcounters", 0, 0, &e);
 *   if (!lmc) {
 *     fprintf(stderr, "Couldn't create localmemcache: %s\n", e.error_str);
 *     return 1;
 *   }
 *   if (!local_memcache_set(lmc, "foo", 3, "1", 1)) goto failed;
 *   size_t n_value;
 *   char *value = local_memcache_get_new(lmc, "foo", 3, &n_value);
 *   if (!value) goto failed;
 *   free(value);
 *   if (!local_memcache_delete(lmc, "foo", 3)) goto failed;
 *   if (!local_memcache_free(lmc, &e)) {
 *     fprintf(stderr, "Failed to release localmemcache: %s\n", e.error_str);
 *     return 1;
 *   }
 * 
 *   return 0;
 * 
 * failed:
 *   fprintf(stderr, "%s\n", lmc->error.error_str);
 *   return 1;
 * 
 * }
 *
 *  == Default sizes of memory pools
 *
 *  The default size for memory pools is 1024 (MB). It cannot be changed later,
 *  so choose a size that will provide enough space for all your data.  You
 *  might consider setting this size to the maximum filesize of your
 *  filesystem.  Also note that while these memory pools may look large on your
 *  disk, they really aren't, because with sparse files only those parts of the
 *  file which contain non-null data actually use disk space.
 *
 *  == Automatic recovery from crashes
 *
 *  In case a process is terminated while accessing a memory pool, other
 *  processes will wait for the lock up to 2 seconds, and will then try to
 *  resume the aborted operation.  This can also be done explicitly by using
 *  LocalMemCache.check(options).
 *
 *  == Clearing memory pools
 *
 *  Removing memory pools can be done with LocalMemCache.clear(options). 
 *
 *  == Environment
 *  
 *  If you use the :namespace parameter, the .lmc file for your namespace will
 *  reside in /var/tmp/localmemcache.  This can be overriden by setting the
 *  LMC_NAMESPACES_ROOT_PATH variable in the environment.
 *
 */

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

/*
 *  Creates a new handle for accessing a shared memory region.
 * 
 *  lmc_error_t e;
 *  // open via namespace
 *  local_memcache_t *lmc = local_memcache_create("viewcounters", 0, 0, &e);
 *  // open via filename
 *  local_memcache_t *lmc = local_memcache_create(0, "./foo.lmc", 0, &e);
 * 
 *  You must supply at least a namespace or filename parameter
 *
 *  The size_mb defaults to 1024 (1 GB).
 *
 *  If you use the namespace parameter, the .lmc file for your namespace will
 *  reside in /var/tmp/localmemcache.  This can be overriden by setting the
 *  LMC_NAMESPACES_ROOT_PATH variable in the environment.
 *
 *  When you first call .new for a previously not existing memory pool, a
 *  sparse file will be created and memory and disk space will be allocated to
 *  hold the empty hashtable (about 100K), so the size_mb refers
 *  only to the maximum size of the memory pool.  .new for an already existing
 *  memory pool will only map the already previously allocated RAM into the
 *  virtual address space of your process.  
 */
local_memcache_t *local_memcache_create(const char *namespace, 
    const char *filename, double size_mb, lmc_error_t* e);

/* 
 *  Retrieve string value from hashtable.
 *
 *  It will return a newly allocated string which you need to free() after use.
 */
char *local_memcache_get_new(local_memcache_t *lmc, const char *key, 
    size_t n_key, size_t *n_value);

/* 
 *  Set string value in hashtable.
 */
int local_memcache_set(local_memcache_t *lmc, const char *key, size_t n_key, 
    const char* value, size_t n_value);

/* 
 *  Deletes key from hashtable.  
 */
int local_memcache_delete(local_memcache_t *lmc, char *key, size_t n_key);

/*
 * Releases memory pool handle.
 */
int local_memcache_free(local_memcache_t *lmc, lmc_error_t *e);

/*
 * Iterate over key value pairs in memory pool
 *
 * example:
 *   typedef struct {
 *     ...
 *   } collector_t;
 *
 *   int my_collect(void *ctx, const char* key, const char* value) {
 *      collector_t *c = ctx;
 *      ....
 *   }
 *
 *   local_memcache_t *lmc;
 *   collector_t c;
 *   local_memcache_iterate(lmc, (void *) &c, my_collect);
 *
 * The memory pool will be locked while iteration takes place, so try to make
 * sure you can iterate within under 2 seconds otherwise other waiting
 * processes will try to remove the lock (2 seconds is the timeout for
 * triggering the automatic recovery.)
 *
 */
int local_memcache_iterate(local_memcache_t *lmc, void *ctx, size_t *ofs,
    LMC_ITERATOR_P(iter));

/*
 * Deletes a memory pool.  If repair is 1, locked semaphores are
 * removed as well.
 *
 * WARNING: Do only call this method with the repair option if you are sure
 * that you really want to remove this memory pool and no more processes are
 * still using it.
 *
 * If you delete a pool and other processes still have handles open on it, the
 * status of these handles becomes undefined.  There's no way for a process to
 * know when a handle is not valid anymore, so only delete a memory pool if
 * you are sure that all handles are closed.
 *
 * The memory pool must be specified by either setting the filename or
 * namespace parameter.  
 */
int local_memcache_clear_namespace(const char *namespace, const char *filename,
    int repair, lmc_error_t *e);
/*
 * Tries to repair a corrupt namespace.  Usually one doesn't call this method
 * directly, it's invoked automatically when operations time out.
 *
 * The memory pool must be specified by either setting the filename or
 * namespace parameter. 
 */
int local_memcache_check_namespace(const char *namespace, const char *filename, 
    lmc_error_t *e);

/* internal, do not use */
const char *__local_memcache_get(local_memcache_t *lmc, 
    const char *key, size_t n_key, size_t *n_value);

/* internal, do not use */
int lmc_unlock_shm_region(const char *who, local_memcache_t *lmc);
#endif
