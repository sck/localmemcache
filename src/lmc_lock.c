#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include "lmc_lock.h"

int c_l(lmc_lock_t *l, lmc_error_t *e) { 
  if (!l) { strncpy(e->error_str, "Semaphore not initialized", 1023); }
  return l != NULL; 
}


int lmc_ref_counter_get_value(lmc_lock_t *l, lmc_error_t *e) {
  if (!c_l(l, e)) return -1;
  int i;
  if (lmc_handle_error(sem_getvalue(l->sem, &i) == -1, "sem_getvalue", e) == -1) 
      return -1;
  return i;
}

int lmc_ref_counter_manip(lmc_lock_t *l, int i, lmc_error_t *e) {
  if (!c_l(l, e)) return 0;
  int v = lmc_ref_counter_get_value(l, e);
  if (v == -1 || lmc_handle_error(sem_getvalue(l->sem, &i) == -1, 
      "sem_getvalue", e) == -1) return 0;
  if (i == -1) {
    if (v <= 0) { 
      strncpy(e->error_str, "Ref counter may not become negative!", 1023); 
      return 0;
    }
    printf("rc: sem_wait\n");
    sem_wait(l->sem);
  } else {
    printf("rc: sem_post\n");
    sem_post(l->sem);
  }
  return 1;
}

int lmc_ref_counter_reset(lmc_lock_t *l, lmc_error_t *e) {
  if (!c_l(l, e)) return 0;
  int v;
  while ( (v = lmc_ref_counter_get_value(l, e)) > 0) {
    lmc_ref_counter_manip(l, -1, e);
  }
  return 1;
}


lmc_lock_t *lmc_lock_init(const char *namespace, int init, lmc_error_t *e) {
  lmc_lock_t *l = malloc(sizeof(lmc_lock_t));
  if (!l) return NULL;
  strncpy((char *)&l->namespace, namespace, 1023);
  
  lmc_handle_error((l->sem = sem_open(l->namespace, O_CREAT, 0600, init)) == NULL,
      "sem_open", e);
  if (!l->sem) { free(l); return NULL; }
  return l;
}

int lmc_is_locked(lmc_lock_t* l, lmc_error_t *e) {
  if (!c_l(l, e)) { return 0; }
  if (sem_trywait(l->sem) == -1) {
    return 1;
  } else {
    sem_post(l->sem);
    return 0;
  }
}

int lmc_is_lock_working(lmc_lock_t* l, lmc_error_t *e) {
  if (!c_l(l, e)) { return 0; }
  struct timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  ts.tv_sec += 2;
  if (sem_timedwait(l->sem, &ts) == -1) {
    return 0;
  } else {
    sem_post(l->sem);
    return 1;
  }
}

void lmc_lock_repair(lmc_lock_t *l) {
  int v; 
  sem_getvalue(l->sem, &v);
  if (v == 0) { sem_post(l->sem); }
  sem_getvalue(l->sem, &v);
  printf("value after repair: %d\n", v);
}

int lmc_lock_obtain(const char *where, lmc_lock_t* l, lmc_error_t *e) {
  return c_l(l,e) && lmc_handle_error(sem_wait(l->sem) == -1, "sem_wait", e);
}

int lmc_lock_release(const char *where, lmc_lock_t* l, lmc_error_t *e) {
  return c_l(l, e) && lmc_handle_error(sem_post(l->sem) == -1, "sem_post", e);
}
