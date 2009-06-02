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

#ifndef RARRAY_PTR
#define RARRAY_PTR(array) array->ptr
#endif

#ifndef RARRAY_LEN
#define RARRAY_LEN(array) array->len
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
char *rstring_ptr_null(VALUE s) { 
  char* r = NIL_P(s) ? NULL : RSTRING_PTR(rb_String(s)); 
  return r ? r : NULL;
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
static VALUE lmc_rb_sym_namespace;
static VALUE lmc_rb_sym_filename;
static VALUE lmc_rb_sym_size_mb;
static VALUE lmc_rb_sym_force;

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
void lmc_check_dict(VALUE o) {
  if (TYPE(o) != T_HASH) {
    rb_raise(rb_eArgError, "expected a Hash");
  }
}

/* :nodoc: */
static VALUE LocalMemCache__new2(VALUE klass, VALUE o) {
  lmc_check_dict(o);
  lmc_error_t e;
  local_memcache_t *l = local_memcache_create(
      rstring_ptr_null(rb_hash_aref(o, lmc_rb_sym_namespace)),
      rstring_ptr_null(rb_hash_aref(o, lmc_rb_sym_filename)), 
      double_value(rb_hash_aref(o, lmc_rb_sym_size_mb)), &e);
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

/*
 * call-seq: LocalMemCache.drop(*args)
 *
 * Deletes a memory pool.  If the :force option is set, locked semaphores are
 * removed as well.
 *
 * WARNING: Do only call this method with the :force option if you are sure
 * that you really want to remove this memory pool and no more processes are
 * still using it.
 *
 * If you delete a pool and other processes still have handles open on it, the
 * status of these handles becomes undefined.  There's no way for a process to
 * know when a handle is not valid anymore, so only delete a memory pool if
 * you are sure that all handles are closed.
 *
 * valid options for drop are 
 * [:namespace] 
 * [:filename] 
 * [:force] 
 *
 * The memory pool must be specified by either setting the :filename or
 * :namespace option.  The default for :force is false.
 */
static VALUE LocalMemCache__drop(VALUE klass, VALUE o) {
  lmc_check_dict(o);
  lmc_error_t e;
  if (!local_memcache_drop_namespace(
      rstring_ptr_null(rb_hash_aref(o, lmc_rb_sym_namespace)), 
      rstring_ptr_null(rb_hash_aref(o, lmc_rb_sym_filename)),
      bool_value(rb_hash_aref(o, lmc_rb_sym_force)), &e)) {
    rb_lmc_raise_exception(&e); 
  }
  return Qnil;
}

/*
 * call-seq: LocalMemCache.check(*args)
 *
 * Tries to repair a corrupt namespace.  Usually one doesn't call this method
 * directly, it's invoked automatically when operations time out.
 *
 * valid options are 
 * [:namespace] 
 * [:filename] 
 *
 * The memory pool must be specified by either setting the :filename or
 * :namespace option. 
 */
static VALUE LocalMemCache__check(VALUE klass, VALUE o) {
  lmc_check_dict(o);
  lmc_error_t e;
  if (!local_memcache_check_namespace(
      rstring_ptr_null(rb_hash_aref(o, lmc_rb_sym_namespace)), 
      rstring_ptr_null(rb_hash_aref(o, lmc_rb_sym_filename)),
      &e)) {
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
 *     lmc.get(key)   ->   string value or nil
 *     lmc[key]       ->   string value or nil
 *
 *  Retrieve string value from hashtable.
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
 *     lmc.random_pair()   ->  [key, value] or nil
 *
 *  Retrieves random pair from hashtable.
 */
static VALUE LocalMemCache__random_pair(VALUE obj) {
  char *k, *v;
  size_t n_k, n_v;
  VALUE r = Qnil;
  if (__local_memcache_random_pair(get_LocalMemCache(obj), &k, &n_k, &v, 
      &n_v)) {
    r = rb_ary_new();
    rb_ary_push(r, lmc_ruby_string2(k, n_k));
    rb_ary_push(r, lmc_ruby_string2(v, n_v));
  }
  lmc_unlock_shm_region("local_memcache_random_pair", 
      get_LocalMemCache(obj));
  return r;
}

/* 
 *  call-seq:
 *     lmc.set(key, value)   ->   Qnil
 *     lmc[key]=value        ->   Qnil
 *
 *  Set value for key in hashtable.  Value and key will be converted to
 *  string.
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
 *     lmc.clear -> Qnil
 *
 *  Clears content of hashtable.
 */
static VALUE LocalMemCache__clear(VALUE obj) {
  local_memcache_t *lmc = get_LocalMemCache(obj);
  if (!local_memcache_clear(lmc)) rb_lmc_raise_exception(&lmc->error); 
  return Qnil;
}

/* 
 *  call-seq:
 *     lmc.delete(key)   ->   Qnil
 *
 *  Deletes key from hashtable.  The key is converted to string.
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
  int success = 2;
  size_t ofs = 0;
  while (success == 2) {
    success = local_memcache_iterate(get_LocalMemCache(obj), 
        (void *) &data, &ofs, lmc_ruby_iter);
  }
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

typedef struct {
  VALUE ary;
} lmc_ruby_iter_collect_pairs_t;

/* :nodoc: */
int lmc_ruby_iter_collect_pairs(void *ctx, const char* key, const char* value) {
  lmc_ruby_iter_collect_pairs_t *data = ctx;
  rb_ary_push(data->ary, rb_assoc_new(lmc_ruby_string(key), 
      lmc_ruby_string(value)));
  return 1;
}

/* :nodoc: */
static VALUE __LocalMemCache__each_pair(VALUE d) {
  VALUE obj = rb_ary_entry(d, 0);
  int success = 2;
  size_t ofs = 0;
  while (success == 2) {
    VALUE r = rb_ary_new();
    lmc_ruby_iter_collect_pairs_t data;
    data.ary = r;
    success = local_memcache_iterate(get_LocalMemCache(obj), 
        (void *) &data, &ofs, lmc_ruby_iter_collect_pairs);
    long i;
    for (i = 0; i < RARRAY_LEN(r); i++) {
      rb_yield(RARRAY_PTR(r)[i]);
    }
  }
  if (!success) { return Qnil; }
  return Qnil;
}

/* 
 *  call-seq:
 *     lmc.each_pair {|k, v|  block } -> nil
 *
 *  Iterates over hashtable.
 */
static VALUE LocalMemCache__each_pair(VALUE obj) {
  VALUE d = rb_ary_new();
  rb_ary_push(d, obj);
  int error = 0;
  rb_protect(__LocalMemCache__each_pair, d, &error);
  if (error) {
    lmc_unlock_shm_region("local_memcache_iterate", get_LocalMemCache(obj));
    rb_exc_raise(ruby_errinfo);
  }
  return Qnil;
}

/* 
 *  call-seq:
 *     lmc.size -> number
 *
 *  Number of pairs in the hashtable.
 */
static VALUE LocalMemCache__size(VALUE obj) {
  local_memcache_t *lmc = get_LocalMemCache(obj);
  ht_hash_t *ht = lmc->base + lmc->va_hash;
  return rb_int2big(ht->size);
}

/*
 * Document-class: LocalMemCache
 * 
 * <code>LocalMemCache</code> provides for a Hashtable of strings in shared
 * memory (via a memory mapped file), which thus can be shared between
 * processes on a computer.  Here is an example of its usage:
 *
 *   $lm = LocalMemCache.new :namespace => "viewcounters"
 *   $lm[:foo] = 1
 *   $lm[:foo]          # -> "1"
 *   $lm.delete(:foo)
 *
 * <code>LocalMemCache</code> can also be used as a persistent key value
 * database, just use the :filename instead of the :namespace parameter.
 *
 *   $lm = LocalMemCache.new :filename => "my-database.lmc"
 *   $lm[:foo] = 1
 *   $lm[:foo]          # -> "1"
 *   $lm.delete(:foo)
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
 *  Removing memory pools can be done with LocalMemCache.drop(options). 
 *
 *  == Environment
 *  
 *  If you use the :namespace parameter, the .lmc file for your namespace will
 *  reside in /var/tmp/localmemcache.  This can be overriden by setting the
 *  LMC_NAMESPACES_ROOT_PATH variable in the environment.
 *
 *  == Storing Ruby Objects
 *
 *  If you want to store Ruby objects instead of just strings, consider 
 *  using LocalMemCache::SharedObjectStorage.
 *
 */
void Init_rblocalmemcache() {
  LocalMemCache = rb_define_class("LocalMemCache", rb_cObject);
  rb_define_singleton_method(LocalMemCache, "_new", LocalMemCache__new2, 1);
  rb_define_singleton_method(LocalMemCache, "drop", 
      LocalMemCache__drop, 1);
  rb_define_singleton_method(LocalMemCache, "check", 
      LocalMemCache__check, 1);
  rb_define_singleton_method(LocalMemCache, "disable_test_crash", 
      LocalMemCache__disable_test_crash, 0);
  rb_define_singleton_method(LocalMemCache, "enable_test_crash", 
      LocalMemCache__enable_test_crash, 0);
  rb_define_method(LocalMemCache, "get", LocalMemCache__get, 1);
  rb_define_method(LocalMemCache, "[]", LocalMemCache__get, 1);
  rb_define_method(LocalMemCache, "delete", LocalMemCache__delete, 1);
  rb_define_method(LocalMemCache, "set", LocalMemCache__set, 2);
  rb_define_method(LocalMemCache, "clear", LocalMemCache__clear, 0);
  rb_define_method(LocalMemCache, "[]=", LocalMemCache__set, 2);
  rb_define_method(LocalMemCache, "keys", LocalMemCache__keys, 0);
  rb_define_method(LocalMemCache, "each_pair", LocalMemCache__each_pair, 0);
  rb_define_method(LocalMemCache, "random_pair", LocalMemCache__random_pair, 
      0);
  rb_define_method(LocalMemCache, "close", LocalMemCache__close, 0);
  rb_define_method(LocalMemCache, "size", LocalMemCache__size, 0);

  lmc_rb_sym_namespace = ID2SYM(rb_intern("namespace"));
  lmc_rb_sym_filename = ID2SYM(rb_intern("filename"));
  lmc_rb_sym_size_mb = ID2SYM(rb_intern("size_mb"));
  lmc_rb_sym_force = ID2SYM(rb_intern("force"));
  rb_require("localmemcache.rb");
}
