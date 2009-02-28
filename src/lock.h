/*
 * Copyright (c) 2009, Sven C. Koehler
 */

#ifndef _LMC_LOCK_H_
#define _LMC_LOCK_H_
#include <semaphore.h>
#include "lmc_error.h"

typedef struct {
  sem_t *sem;
  char namespace[1024];
} lmc_lock_t;

lmc_lock_t *lmc_lock_init(const char *namespace, lmc_error_t *e);
int lmc_lock_obtain(const char *where, lmc_lock_t* l, lmc_error_t *e);
int lmc_lock_release(const char *where, lmc_lock_t* l, lmc_error_t *e);

int lmc_is_lock_working(lmc_lock_t* l, lmc_error_t *e);
void lmc_lock_repair(lmc_lock_t *l);
#endif
