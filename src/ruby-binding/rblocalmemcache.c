/*
 * Copyright (C) 2009, Sven C. Koehler
 */

#include <ruby.h>
#include <sys/types.h>
#include <unistd.h>
#include "localmemcache.h"

#ifndef RSTRING_LEN
#define RSTRING_LEN(x) RSTRING(x)->len
#endif

#ifndef RSTRING_PTR
#define RSTRING_PTR(x) RSTRING(x)->ptr
#endif


#if RUBY_VERSION_CODE >= 190
#define ruby_errinfo rb_errinfo()
#endif

/* :nodoc: */
long long_value(VALUE i) { return NUM2LONG(rb_Integer(i)); }
/* :nodoc: */
double double_value(VALUE i) { return NUM2DBL(i); }
/* :nodoc: */
VALUE num2string(long i) { return rb_big2str(rb_int2big(i), 10); }
/* :nodoc: */
char *rstring_ptr(VALUE s) { 
  char* r = NIL_P(s) ? "nil" : RSTRING_PTR(rb_String(s)); 
  return r ? r : "nil";
}

/* :nodoc: */
size_t rstring_length(VALUE s) { 
  size_t r = NIL_P(s) ? 0 : RSTRING_LEN(rb_String(s)); 
  return r;
}
/* :nodoc: */
static VALUE ruby_string(const char *s) { return s ? rb_str_new2(s) : Qnil; }
/* :nodoc: */
int bool_value(VALUE v) { return v == Qtrue; }

/* :nodoc: */
static VALUE lmc_ruby_string2(const char *s, size_t l) { 
  return s ? rb_str_new(s, l) : Qnil; 
}

/* :nodoc: */
static VALUE lmc_ruby_string(const char *s) { 
  return lmc_ruby_string2(s + sizeof(size_t), *(size_t *) s);
}

typedef struct {
  local_memcache_t *lmc;
  int open;
} rb_lmc_handle_t;

static VALUE LocalMemCache;

/* :nodoc: */
void __rb_lmc_raise_exception(const char *error_type, const char *m) {
  VALUE eid = rb_intern(error_type);
  VALUE k = rb_const_get(LocalMemCache, eid);
  rb_raise(k, m);
}

/* :nodoc: */
void rb_lmc_raise_exception(lmc_error_t *e) {
  __rb_lmc_raise_exception(e->error_type, e->error_str);
}

/* :nodoc: */
local_memcache_t *rb_lmc_check_handle_access(rb_lmc_handle_t *h) {
  if (!h || (h->open == 0) || !h->lmc) {
    __rb_lmc_raise_exception("MemoryPoolClosed", "Pool is closed");
    return 0;
  }
  return h->lmc;
}

/* :nodoc: */
static void rb_lmc_free_handle(rb_lmc_handle_t *h) {
  lmc_error_t e;
  local_memcache_free(rb_lmc_check_handle_access(h), &e);
}


/* :nodoc: */
static VALUE LocalMemCache__new2(VALUE klass, VALUE namespace, VALUE size_mb) {
  lmc_error_t e;
  local_memcache_t *l = local_memcache_create(rstring_ptr(namespace), 
      double_value(size_mb), &e);
  if (!l)  rb_lmc_raise_exception(&e);
  rb_lmc_handle_t *h = calloc(1, sizeof(rb_lmc_handle_t));
  if (!h) rb_raise(rb_eRuntimeError, "memory allocation error");
  h->lmc = l;
  h->open = 1;
  return Data_Wrap_Struct(klass, NULL, rb_lmc_free_handle, h);
}

/* :nodoc: */
local_memcache_t *get_LocalMemCache(VALUE obj) {
  rb_lmc_handle_t *h;
  Data_Get_Struct(obj, rb_lmc_handle_t, h);
  return rb_lmc_check_handle_access(h);
}


/* :nodoc: */
static VALUE LocalMemCache__clear_namespace(VALUE klass, VALUE ns, 
    VALUE repair) {
  lmc_error_t e;
  if (!local_memcache_clear_namespace(rstring_ptr(ns), bool_value(repair), &e)) {
    rb_lmc_raise_exception(&e); 
  }
  return Qnil;
}

/* :nodoc: */
static VALUE LocalMemCache__check_namespace(VALUE klass, VALUE ns) {
  lmc_error_t e;
  if (!local_memcache_check_namespace(rstring_ptr(ns), &e)) {
    rb_lmc_raise_exception(&e); 
  }
  return Qnil;
}

/* :nodoc: */
static VALUE LocalMemCache__enable_test_crash(VALUE klass) {
  srand(getpid());
  lmc_test_crash_enabled = 1;
  return Qnil;
}

/* :nodoc: */
static VALUE LocalMemCache__disable_test_crash(VALUE klass) {
  lmc_test_crash_enabled = 0;
  return Qnil;
}


/* 
 *  call-seq:
 *     lmc.get(key)   ->   Qnil
 *     lmc[key]       ->   Qnil
 *
 *  Retrieve value from hashtable.
 */
static VALUE LocalMemCache__get(VALUE obj, VALUE key) {
  size_t l;
  const char* r = __local_memcache_get(get_LocalMemCache(obj), 
      rstring_ptr(key), rstring_length(key), &l);
  VALUE rr = lmc_ruby_string2(r, l);
  lmc_unlock_shm_region("local_memcache_get", get_LocalMemCache(obj));
  return rr;
}

/* 
 *  call-seq:
 *     lmc.set(key, value)   ->   Qnil
 *     lmc[key]=value        ->   Qnil
 *
 *  Set value for key in hashtable.
 */

static VALUE LocalMemCache__set(VALUE obj, VALUE key, VALUE value) {
  local_memcache_t *lmc = get_LocalMemCache(obj);
  if (!local_memcache_set(lmc, rstring_ptr(key), rstring_length(key), 
      rstring_ptr(value), rstring_length(value))) { 
    rb_lmc_raise_exception(&lmc->error); 
  }
  return Qnil;
}

/* 
 *  call-seq:
 *     lmc.delete(key)   ->   Qnil
 *
 *  Deletes key from hashtable.
 */
static VALUE LocalMemCache__delete(VALUE obj, VALUE key) {
  return local_memcache_delete(get_LocalMemCache(obj), 
      rstring_ptr(key), rstring_length(key));
  return Qnil;
}

/* 
 *  call-seq:
 *     lmc.close()   ->   Qnil
 *
 *  Releases hashtable.
 */
static VALUE LocalMemCache__close(VALUE obj) {
  lmc_error_t e;
  rb_lmc_handle_t *h;
  Data_Get_Struct(obj, rb_lmc_handle_t, h);
  if (!local_memcache_free(rb_lmc_check_handle_access(h), &e)) 
      rb_lmc_raise_exception(&e);
  h->open = 0;
  return Qnil;
}

typedef struct {
  VALUE ary;
} lmc_ruby_iter_collect_keys;

/* :nodoc: */
int lmc_ruby_iter(void *ctx, const char* key, const char* value) {
  lmc_ruby_iter_collect_keys *data = ctx;
  rb_ary_push(data->ary, lmc_ruby_string(key));
  return 1;
}

/* :nodoc: */
static VALUE __LocalMemCache__keys(VALUE d) {
  VALUE obj = rb_ary_entry(d, 0);
  VALUE r = rb_ary_entry(d, 1);
  lmc_ruby_iter_collect_keys data;
  data.ary = r;
  int success = local_memcache_iterate(get_LocalMemCache(obj), 
      (void *) &data, lmc_ruby_iter);
  if (!success) { return Qnil; }
  return Qnil;
}

/* 
 *  call-seq:
 *     lmc.keys()   ->   array or nil
 *
 *  Returns a list of keys.
 */
static VALUE LocalMemCache__keys(VALUE obj) {
  VALUE d = rb_ary_new();
  rb_ary_push(d, obj);
  rb_ary_push(d, rb_ary_new());
  int error = 0;
  rb_protect(__LocalMemCache__keys, d, &error);
  if (error) {
    lmc_unlock_shm_region("local_memcache_iterate", get_LocalMemCache(obj));
    rb_exc_raise(ruby_errinfo);
  }
  return rb_ary_entry(d, 1);
}

void Init_rblocalmemcache() {
  LocalMemCache = rb_define_class("LocalMemCache", rb_cObject);
  rb_define_singleton_method(LocalMemCache, "_new", LocalMemCache__new2, 2);
  rb_define_singleton_method(LocalMemCache, "_clear_namespace", 
      LocalMemCache__clear_namespace, 2);
  rb_define_singleton_method(LocalMemCache, "check_namespace", 
      LocalMemCache__check_namespace, 1);
  rb_define_singleton_method(LocalMemCache, "disable_test_crash", 
      LocalMemCache__disable_test_crash, 0);
  rb_define_singleton_method(LocalMemCache, "enable_test_crash", 
      LocalMemCache__enable_test_crash, 0);
  rb_define_method(LocalMemCache, "get", LocalMemCache__get, 1);
  rb_define_method(LocalMemCache, "[]", LocalMemCache__get, 1);
  rb_define_method(LocalMemCache, "delete", LocalMemCache__delete, 1);
  rb_define_method(LocalMemCache, "set", LocalMemCache__set, 2);
  rb_define_method(LocalMemCache, "[]=", LocalMemCache__set, 2);
  rb_define_method(LocalMemCache, "keys", LocalMemCache__keys, 0);
  rb_define_method(LocalMemCache, "close", LocalMemCache__close, 0);
}
