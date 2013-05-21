/*
 * Copyright (c) 2009, Sven C. Koehler
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include "lmc_lock.h"
#include "lmc_common.h"

void lmc_namespacify(char *result, const char *s) {
  char cs[1024];
  if (lmc_is_filename(s)) { lmc_clean_string(cs, s); }
  else { strcpy(cs, s); }
  snprintf(result, 15, "/lmc-%zX", lmc_hash(cs, strlen(cs))) ;
}

lmc_lock_t *lmc_lock_init(const char *ns, int init, lmc_error_t *e) {
  char namespace[1024];
  lmc_namespacify(namespace, ns);
  lmc_lock_t *l = malloc(sizeof(lmc_lock_t));
  if (!l) return NULL;
  snprintf((char *)&l->namespace, 1023, "%s", namespace);
  lmc_handle_error((l->sem = sem_open(l->namespace, O_CREAT, 0600, init)) == 
      NULL, "sem_open", "LockError", l->namespace, e);
  if (!l->sem) { free(l); return NULL; }
  return l;
}

void lmc_lock_free(lmc_lock_t* l) {
  if (!l) return;
  if (l->sem) { sem_close(l->sem); }
  free(l);
}

int lmc_clear_namespace_lock(const char *ns) {
  char namespace[1024];
  lmc_namespacify(namespace, ns);
  lmc_error_t e;
  lmc_lock_t *l = lmc_lock_init(namespace, 1, &e);
  lmc_lock_repair(l);
  lmc_lock_free(l);
  return 1;
}

int lmc_is_locked(lmc_lock_t* l, lmc_error_t *e) {
  if (sem_trywait(l->sem) == -1) {
    return 1;
  } else {
    sem_post(l->sem);
    return 0;
  }
}

int lmc_sem_timed_wait(lmc_lock_t* l) {
#ifdef __APPLE__
  return sem_wait(l->sem);
#else
  struct timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
#ifdef DO_TEST_CRASH
  ts.tv_sec += 1;
#else
  ts.tv_sec += 2;
#endif
  return sem_timedwait(l->sem, &ts);
#endif
}

int lmc_sem_timed_wait_mandatory(lmc_lock_t* l) {
#ifdef __APPLE__
  return sem_wait(l->sem);
#else
  struct timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  ts.tv_sec += 60;
  return sem_timedwait(l->sem, &ts);
#endif
}

int lmc_is_lock_working(lmc_lock_t* l, lmc_error_t *e) {
  if (lmc_sem_timed_wait(l) == -1) {
    return 0;
  } else {
    sem_post(l->sem);
    return 1;
  }
}

void lmc_lock_repair(lmc_lock_t *l) {
#ifdef __APPLE__
  return;
#else
  int v; 
  sem_getvalue(l->sem, &v);
  if (v == 0) { 
    sem_post(l->sem); 
  }
  sem_getvalue(l->sem, &v);
  while (v > 1) { 
    sem_wait(l->sem); 
    sem_getvalue(l->sem, &v);
  }
#endif
}

int lmc_lock_get_value(lmc_lock_t* l) {
  int v = 0;
  sem_getvalue(l->sem, &v);
  return v;
}

int lmc_lock_obtain(const char *where, lmc_lock_t* l, lmc_error_t *e) {
  if (sem_trywait(l->sem) != -1) { return 1; }
  int r = lmc_sem_timed_wait(l);
  if (r == -1 && errno == ETIMEDOUT) {
    lmc_handle_error_with_err_string("sem_timedwait", strerror(errno), 
        "LockTimedOut", 0, e);
    return 0;
  }
  return lmc_handle_error(r, "sem_timedwait", "LockError", l->namespace, e);
}

int lmc_lock_obtain_mandatory(const char *where, lmc_lock_t* l, lmc_error_t *e) {
  int r = lmc_sem_timed_wait_mandatory(l);
  if (r == -1 && errno == ETIMEDOUT) {
    lmc_handle_error_with_err_string("sem_timedwait", strerror(errno), 
        "LockTimedOut", 0, e);
    return 0;
  }
  return lmc_handle_error(r, "sem_wait", "LockError", l->namespace, e);
}

int lmc_lock_release(const char *where, lmc_lock_t* l, lmc_error_t *e) {
  return lmc_handle_error(sem_post(l->sem) == -1, "sem_post", "LockError", 
      l->namespace, e);
}
