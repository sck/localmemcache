/*
 * Copyright (c) 2009, Sven C. Koehler
 */

#ifndef _LMC_SHM_H_INCLUDED_
#define _LMC_SHM_H_INCLUDED_
#include "lmc_error.h"

typedef struct {
  int fd;
  void *base;
  size_t size;
  int use_persistence;
  char namespace[1024];
} lmc_shm_t;

lmc_shm_t *lmc_shm_create(const char *namespace, size_t size, int use_persistence,
    lmc_error_t *e);
int lmc_shm_destroy(lmc_shm_t *mc, lmc_error_t *e);
int lmc_does_namespace_exist(const char *ns);
int lmc_namespace_size(const char *ns);
int lmc_clean_namespace(const char *ns, lmc_error_t *e);
#endif
