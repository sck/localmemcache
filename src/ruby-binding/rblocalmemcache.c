/*
 * Copyright (C) 2009, Sven C. Koehler
 */

#include <ruby.h>
#include "localmemcache.h"

/* :nodoc: */
long long_value(VALUE i) { return NUM2LONG(rb_Integer(i)); }
/* :nodoc: */
VALUE num2string(long i) { return rb_big2str(rb_int2big(i), 10); }
/* :nodoc: */
char *rstring_ptr(VALUE s) { 
  char* r = NIL_P(s) ? "nil" : RSTRING_PTR(rb_String(s)); 
  return r ? r : "nil";
}
/* :nodoc: */
static VALUE ruby_string(const char *s) { return s ? rb_str_new2(s) : Qnil; }
/* :nodoc: */
int bool_value(VALUE v) { return v == Qtrue; }

static VALUE LocalMemCacheError;

/* :nodoc: */
void raise_exception(VALUE error_klass, lmc_error_t *e) {
  rb_raise(error_klass, e->error_str);
}

/* :nodoc: */
static VALUE LocalMemCache__new2(VALUE klass, VALUE namespace, VALUE size) {
  lmc_error_t e;
  local_memcache_t *lmc = local_memcache_create(rstring_ptr(namespace), 
      long_value(size), &e);
  if (!lmc) { raise_exception(LocalMemCacheError, &e); }
  return Data_Wrap_Struct(klass, NULL, local_memcache_free, lmc);
}

/* :nodoc: */
static VALUE LocalMemCache__clear_namespace(VALUE klass, VALUE ns, 
    VALUE repair) {
  lmc_error_t e;
  if (!local_memcache_clear_namespace(rstring_ptr(ns), bool_value(repair), &e)) {
    raise_exception(LocalMemCacheError, &e); 
  }
  return Qnil;
}

/* :nodoc: */
static VALUE LocalMemCache__check_namespace(VALUE klass, VALUE ns) {
  lmc_error_t e;
  if (!local_memcache_check_namespace(rstring_ptr(ns), &e)) {
    raise_exception(LocalMemCacheError, &e); 
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

/* :nodoc: */
local_memcache_t *get_LocalMemCache(VALUE obj) {
  local_memcache_t *lmc;
  Data_Get_Struct(obj, local_memcache_t, lmc);
  return lmc;
}

/* 
 *  call-seq:
 *     lmc.get(key)   ->   Qnil
 *     lmc[key]       ->   Qnil
 *
 *  Retrieve value from hashtable.
 */
static VALUE LocalMemCache__get(VALUE obj, VALUE key) {
  return ruby_string(local_memcache_get(get_LocalMemCache(obj), 
      rstring_ptr(key)));
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
  if (!local_memcache_set(lmc, rstring_ptr(key), rstring_ptr(value))) { 
    raise_exception(LocalMemCacheError, &lmc->error); 
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
      rstring_ptr(key));
  return Qnil;
}

/* 
 *  call-seq:
 *     lmc.close()   ->   Qnil
 *
 *  Releases hashtable.
 */
static VALUE LocalMemCache__close(VALUE obj) {
  local_memcache_free(get_LocalMemCache(obj));
  return Qnil;
}

typedef struct {
  VALUE ary;
} lmc_ruby_iter_collect_keys;

int lmc_ruby_iter(void *ctx, const char* key, const char* value) {
  lmc_ruby_iter_collect_keys *data = ctx;
  rb_ary_push(data->ary, ruby_string(key));
  return 1;
}

/* 
 *  call-seq:
 *     lmc.keys()   ->   array or nil
 *
 *  Returns a list of keys.
 */
static VALUE LocalMemCache__keys(VALUE obj) {
  VALUE r = rb_ary_new();
  lmc_ruby_iter_collect_keys data;
  data.ary = r;
  int success = local_memcache_iterate(get_LocalMemCache(obj), 
      (void *) &data, lmc_ruby_iter);
  if (!success) { return Qnil; }
  return r;
}


static VALUE LocalMemCache;

void Init_rblocalmemcache() {
  LocalMemCacheError = rb_define_class("LocalMemCacheError", rb_eStandardError);
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
