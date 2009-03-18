/*
 * Copyright (c) 2009, Sven C. Koehler
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

#include "lmc_shm.h"

#define LMC_SHM_ROOT_PATH "/var/tmp/localmemcache"

int lmc_does_file_exist(const char *fn) {
  struct stat st;
  return stat(fn, &st) != -1;
}

int lmc_file_size(const char *fn) {
  struct stat st;
  if (stat(fn, &st) == -1) return 0;
  return st.st_size;
}

void lmc_shm_ensure_root_path() {
  if (!lmc_does_file_exist(LMC_SHM_ROOT_PATH)) {
    mkdir(LMC_SHM_ROOT_PATH, 01777);
  }
}

void lmc_file_path_for_namespace(char *result, const char *ns) {
  snprintf(result, 1023, "%s/%s.lmc", LMC_SHM_ROOT_PATH, ns);
}

int lmc_does_namespace_exist(const char *ns) {
  char fn[1024];
  lmc_file_path_for_namespace((char *)&fn, ns);
  return lmc_does_file_exist(fn);
}

int lmc_namespace_size(const char *ns) {
  char fn[1024];
  lmc_file_path_for_namespace((char *)&fn, ns);
  if (!lmc_does_file_exist(fn)) { return 0; }
  return lmc_file_size(fn);
}


int lmc_clean_namespace(const char *ns, lmc_error_t *e) {
  lmc_shm_ensure_root_path();
  char fn[1024];
  lmc_file_path_for_namespace((char *)&fn, ns);
  if (lmc_does_namespace_exist(ns)) {
    if (!lmc_handle_error(unlink(fn) == -1,  "unlink", "ShmError", 
        e)) { return 0; }
  }
  return 1;
}

void lmc_shm_ensure_namespace_file(const char *ns) {
  lmc_shm_ensure_root_path();
  char fn[1024];
  lmc_file_path_for_namespace((char *)&fn, ns);
  if (!lmc_does_namespace_exist(ns)) { close(open(fn, O_CREAT, 0777)); }
}

lmc_shm_t *lmc_shm_create(const char* namespace, size_t size, int use_persistence,
    lmc_error_t *e) {
  lmc_shm_t *mc = calloc(1, sizeof(lmc_shm_t));
  if (!mc) { 
    STD_OUT_OF_MEMORY_ERROR("lmc_shm_create");
    //lmc_handle_error_with_err_string("lmc_shm_create", "Out of memory", e);
    return NULL; 
  }
  strncpy((char *)&mc->namespace, namespace, 1023);
  mc->use_persistence = 0;
  mc->size = size;

  lmc_shm_ensure_namespace_file(mc->namespace);
  char fn[1024];
  lmc_file_path_for_namespace((char *)&fn, mc->namespace);
  if (!lmc_handle_error((mc->fd = open(fn, O_RDWR, (mode_t)0777)) == -1, 
      "open", "ShmError", e)) goto open_failed;
  if (!lmc_handle_error(lseek(mc->fd, mc->size - 1, SEEK_SET) == -1, 
      "lseek", "ShmError", e)) goto failed;
  if (!lmc_handle_error(write(mc->fd, "", 1) != 1, "write", 
      "ShmError", e)) goto failed;
  mc->base = mmap(0, mc->size, PROT_READ | PROT_WRITE, MAP_SHARED, mc->fd, 
      (off_t)0);
  if (!lmc_handle_error(mc->base == MAP_FAILED, "mmap", "ShmError", e)) 
     goto failed;
  return mc;

failed:
  close(mc->fd);
open_failed:
  free(mc);
  return NULL;
}

int lmc_shm_destroy(lmc_shm_t *mc, lmc_error_t *e) {
  int r = lmc_handle_error(munmap(mc->base, mc->size) == -1, "munmap", 
      "ShmError", e);
  close(mc->fd);
  free(mc);
  return r;
}
