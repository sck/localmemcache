/*
 * Copyright (C) 2009, Sven C. Koehler
 */

#include <ruby.h>
#include "lmc_shm.h"
#include "lmc_hashtable.h"
#include "lmc_valloc.h"

#ifndef RSTRING_LEN
#define RSTRING_LEN(x) RSTRING(x)->len
#endif

#ifndef RSTRING_PTR
#define RSTRING_PTR(x) RSTRING(x)->ptr
#endif

void *memp = NULL;
static VALUE OutOfMemoryError;

long long_value(VALUE i) { return NUM2LONG(rb_Integer(i)); }
VALUE num2string(long i) { return rb_big2str(rb_int2big(i), 10); }
char *rstring_ptr(VALUE s) { 
  char* r = NIL_P(s) ? "nil" : RSTRING_PTR(rb_String(s)); 
  return r ? r : "nil";
}
static VALUE ruby_string(char *s) { return s ? rb_str_new2(s) : Qnil; }

static VALUE Alloc__new(VALUE klass, VALUE size) {
  size_t s = long_value(size);
  memp = malloc(s);
#ifdef DEBUG_ALLOC
  printf("memp: %0x, end: %0x\n", memp, memp+s);
  memset(memp, 0xF0, s);
#endif
  lmc_init_memory(memp, s);
  return rb_class_new_instance(0, NULL, klass);
}

static VALUE Alloc__get(VALUE obj, VALUE size) {
  size_t va = lmc_valloc(memp, long_value(size));
  if (!va) { rb_raise(OutOfMemoryError, "Out of memory"); }
  size_t v = long_value(size);
#ifdef DEBUG_ALLOC
  memset(memp + va, va, v);
#endif
  return rb_int2big(va);
}

static VALUE Alloc__dispose(VALUE obj, VALUE adr) {
  lmc_free(memp, long_value(adr));
  return Qnil;
}

static VALUE Alloc__dump(VALUE obj) {
  lmc_dump(memp);
  return Qnil;
}

static VALUE Alloc__free_mem(VALUE obj) {
  return rb_int2inum(lmc_status(memp, "f").free_mem);
}

static VALUE Alloc__largest_chunk(VALUE obj) {
  return rb_int2inum(lmc_status(memp, "lc").largest_chunk);
}

static VALUE Alloc__free_chunks(VALUE obj) {
  return rb_int2inum(lmc_status(memp, "fc").free_chunks);
}

static VALUE SHMError;

static lmc_shm_t* get_Shm(VALUE obj) {
  lmc_shm_t *ls;
  Data_Get_Struct(obj, lmc_shm_t, ls);
  if (ls == NULL) { rb_raise(SHMError, "Not active"); }
  return ls;
}

static VALUE SHM__new(VALUE klass, VALUE namespace) {
  Check_Type(namespace, T_STRING);
  lmc_shm_t *ls = lmc_shm_create("test2", 2000000, 0); 
  return Data_Wrap_Struct(klass, NULL, lmc_shm_destroy, ls);
}

static VALUE SHM__close(VALUE obj) {
  lmc_shm_destroy(get_Shm(obj), 0);
  DATA_PTR(obj) = NULL;
  return Qnil;
}

static VALUE SHM__get(VALUE obj, VALUE id) {
  return num2string(((int *)get_Shm(obj)->base)[long_value(id)]);
}

static VALUE SHM__set(VALUE obj, VALUE id, VALUE value) {
  ((int *)get_Shm(obj)->base)[long_value(id)] = long_value(value);
  return Qnil;
}

//typedef struct  { 
//  va_ht_hash_t va; 
//  mem_cache_t *mc;
//  lmc_lock_t *ll;
//} ht_desc_t;
//
//static VALUE Hashtable__new(VALUE klass) {
//  lmc_lock_obtain("Hashtable__new");
//  size_t s = 20401094656;
//  lmc_lock_t *ll = lmc_lock_init("test11");
//  mem_cache_t *mc = local_mem_cache_create(s); // namespace?
//  lmc_init_memory(mc->shm, s);
//  va_ht_hash_t va_ht = ht_hash_create(mc->shm); 
//  ht_desc_t* ht = malloc(sizeof(ht_desc_t));
//  ht->va = va_ht;
//  ht->mc = mc;
//  ht->ll = ll;
//  lmc_lock_release("Hashtable__new");
//  return Data_Wrap_Struct(klass, NULL, NULL, ht);
//}
//
//ht_desc_t *get_Hashtable(VALUE obj) {
//  ht_desc_t *ht;
//  Data_Get_Struct(obj, ht_desc_t, ht);
//  return ht;
//}
//
//static VALUE Hashtable__get(VALUE obj, VALUE key) {
//  ht_desc_t *ht = get_Hashtable(obj);
//  return ruby_string(ht_get(ht->mc->shm, ht->va, rstring_ptr(key)));
//}
//
//static VALUE Hashtable__set(VALUE obj, VALUE key, VALUE value) {
//  ht_desc_t *ht = get_Hashtable(obj);
//  ht_set(ht->mc->shm, ht->va, rstring_ptr(key), rstring_ptr(value));
//  return Qnil;
//}

static VALUE Alloc;
static VALUE SHM;
//static VALUE Hashtable;

#include <stdio.h>
#include <fcntl.h>     
#include <sys/stat.h>   
#include <sys/mman.h>   

void Init_lmctestapi() {
  OutOfMemoryError = rb_define_class("OutOfMemoryError", rb_eStandardError);
  Alloc = rb_define_class("Alloc", rb_cObject);
  rb_define_singleton_method(Alloc, "new", Alloc__new, 1);
  rb_define_method(Alloc, "get", Alloc__get, 1);
  rb_define_method(Alloc, "dispose", Alloc__dispose, 1);
  rb_define_method(Alloc, "dump", Alloc__dump, 0);
  rb_define_method(Alloc, "free_mem", Alloc__free_mem, 0);
  rb_define_method(Alloc, "largest_chunk", Alloc__largest_chunk, 0);
  rb_define_method(Alloc, "free_chunks", Alloc__free_chunks, 0);

  SHM = rb_define_class("SHM", rb_cObject);
  rb_define_singleton_method(SHM, "new", SHM__new, 1);
  rb_define_method(SHM, "get", SHM__get, 1);
  rb_define_method(SHM, "set", SHM__set, 2);
  rb_define_method(SHM, "close", SHM__close, 0);

  //Hashtable = rb_define_class("Hashtable", rb_cObject);
  //rb_define_singleton_method(Hashtable, "new", Hashtable__new, 0);
  //rb_define_method(Hashtable, "get", Hashtable__get, 1);
  //rb_define_method(Hashtable, "set", Hashtable__set, 2);
}
