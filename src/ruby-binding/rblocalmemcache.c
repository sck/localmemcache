/*
 * Copyright (C) 2009, Sven C. Koehler
 */

#include <ruby.h>
#include "localmemcache.h"

long long_value(VALUE i) { return NUM2LONG(rb_Integer(i)); }
VALUE num2string(long i) { return rb_big2str(rb_int2big(i), 10); }
char *rstring_ptr(VALUE s) { 
  char* r = NIL_P(s) ? "nil" : RSTRING_PTR(rb_String(s)); 
  return r ? r : "nil";
}
static VALUE ruby_string(char *s) { return s ? rb_str_new2(s) : Qnil; }

static VALUE LocalMemCacheError;

void raise_exception(VALUE error_klass, lmc_error_t *e) {
  rb_raise(error_klass, e->error_str);
}

static VALUE LocalMemCache__new2(VALUE klass, VALUE namespace, VALUE size) {
  lmc_error_t e;
  local_memcache_t *lmc = local_memcache_create(rstring_ptr(namespace), 
      long_value(size), &e);
  if (!lmc) { raise_exception(LocalMemCacheError, &e); }
  return Data_Wrap_Struct(klass, NULL, local_memcache_free, lmc);
}

local_memcache_t *get_LocalMemCache(VALUE obj) {
  local_memcache_t *lmc;
  Data_Get_Struct(obj, local_memcache_t, lmc);
  return lmc;
}

static VALUE LocalMemCache__get(VALUE obj, VALUE key) {
  return ruby_string(local_memcache_get(get_LocalMemCache(obj), 
      rstring_ptr(key)));
}

static VALUE LocalMemCache__set(VALUE obj, VALUE key, VALUE value) {
  local_memcache_t *lmc = get_LocalMemCache(obj);
  if (!local_memcache_set(lmc, rstring_ptr(key), rstring_ptr(value))) { 
    raise_exception(LocalMemCacheError, &lmc->error); 
  }
  return Qnil;
}

static VALUE LocalMemCache__close(VALUE obj) {
  local_memcache_free(get_LocalMemCache(obj));
  return Qnil;
}

static VALUE LocalMemCache;

void Init_rblocalmemcache() {
  LocalMemCacheError = rb_define_class("LocalMemCacheError", rb_eStandardError);
  LocalMemCache = rb_define_class("LocalMemCache", rb_cObject);
  rb_define_singleton_method(LocalMemCache, "_new", LocalMemCache__new2, 2);
  rb_define_method(LocalMemCache, "get", LocalMemCache__get, 1);
  rb_define_method(LocalMemCache, "set", LocalMemCache__set, 2);
  rb_define_method(LocalMemCache, "close", LocalMemCache__close, 0);
}
