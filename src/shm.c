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

#include "shm.h"

#define LMC_SHM_ROOT_PATH "/var/tmp/localmemcache"

int lmc_does_file_exist(const char *fn) {
  struct stat st;
  return stat(fn, &st) != -1;
}

void lmc_shm_ensure_root_path() {
  if (!lmc_does_file_exist(LMC_SHM_ROOT_PATH)) {
    mkdir(LMC_SHM_ROOT_PATH, 01777);
  }
}

void lmc_shm_ensure_namespace_file(char *ns) {
  lmc_shm_ensure_root_path();
  char fn[1024];
  snprintf(fn, 1023, "%s/%s.lmc", LMC_SHM_ROOT_PATH, ns);
  if (!lmc_does_file_exist(fn)) { close(open(fn, O_CREAT, 0777)); }
}

lmc_shm_t *lmc_shm_create(char* namespace, size_t size, int use_persistence,
    lmc_error_t *e) {
  lmc_shm_t *mc = calloc(1, sizeof(lmc_shm_t));
  if (!mc) { 
    lmc_handle_error_with_err_string("lmc_shm_create", "Out of memory", e);
    return NULL; 
  }
  strncpy((char *)&mc->namespace, namespace, 1023);
  mc->use_persistence = 0;
  mc->size = size;

  lmc_shm_ensure_namespace_file(mc->namespace);
  char fn[1024];
  snprintf(fn, 1023, "%s/%s.lmc", LMC_SHM_ROOT_PATH, mc->namespace);

//#define FILESIZE 2000
//  int fd = open(fn, O_RDWR, (mode_t)0600);
//  if (fd==-1) { printf("e:o\n"); }
//  int r = lseek(fd, mc->size-1, SEEK_SET);
//  if (r==-1) { printf("e:s\n"); }
//  r = write(fd, "", 1);
//  if (r!=1) { printf("e:w\n"); }
//  void *b = mmap(0, mc->size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
//  printf("shm: %x\n", b);
//  if (b == MAP_FAILED) { printf("e:m\n"); }
//
//  exit(0);

  //char fn[1024];
  snprintf(fn, 1023, "%s/%s.lmc", LMC_SHM_ROOT_PATH, mc->namespace);
  if (!lmc_handle_error((mc->fd = open(fn, O_RDWR, (mode_t)0777)) == -1, 
      "open", e)) goto open_failed;
  if (!lmc_handle_error(lseek(mc->fd, mc->size - 1, SEEK_SET) == -1, "lseek", e)) 
      goto failed;
  if (!lmc_handle_error(write(mc->fd, "", 1) != 1, "write", e)) goto failed;
  //} else {
  //char n[1024];
  //snprintf(n, 1023, "/lmc-%s", mc->namespace);
  //printf("shm2\n");
  //mc->fd = shm_open(n, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
  //if (!lmc_handle_error(mc->fd == -1, "shm_open", e)) goto failed;
  //printf("shm2.5\n");

  //if (!lmc_handle_error(ftruncate(mc->fd, mc->size) == -1, "ftruncate", e))
  //    goto failed;
  //printf("shm3\n");

  mc->base = mmap(0, mc->size, PROT_READ | PROT_WRITE, MAP_SHARED, mc->fd, 
      (off_t)0);
  if (!lmc_handle_error(mc->base == MAP_FAILED, "mmap", e))  goto failed;
  return mc;

failed:
  close(mc->fd);
open_failed:
  free(mc);
  return NULL;
}

int lmc_shm_destroy(lmc_shm_t *mc, lmc_error_t *e) {
  int r = lmc_handle_error(munmap(mc->base, mc->size) == -1, "munmap", e);
  close(mc->fd);
  free(mc);
  return r;
}
