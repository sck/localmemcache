#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

typedef struct {
  size_t first_free;
  size_t dummy2;
  size_t total_size;
} MemDescriptor;

typedef struct {
  size_t next;
  size_t size;
} MemChunkDescriptor;

typedef struct {
  size_t free_chunks;
  size_t total_mem;
  size_t total_free_mem;
  size_t free_mem;
  size_t largest_chunk;
} MemStatus;

MemChunkDescriptor *md_first_free(void *base) {
  MemDescriptor *md = base;
  return md->first_free == 0 ? 0 : base + md->first_free;
}

void lmc_dump_chunk(void *base, MemChunkDescriptor* c) {
  size_t va_c = (void *)c - base;
  printf("chunk %d:\n"
      "  start: %d\n"
      "  end  : %d\n"
      "  size : %d\n"
      "  next : %d\n"
      "  ------------------------\n"
      , va_c, va_c, va_c + c->size, c->size, c->next);
}

void lmc_dump_chunk_brief(char *who, void *base, MemChunkDescriptor* c) {
  if (!c) { return; }
  size_t va_c = (void *)c - base;
  printf("[%s] chunk %d:\n", who, va_c);
}


void lmc_dump(void *base) {
  MemDescriptor *md = base;
  MemChunkDescriptor* c = md_first_free(base);
  MemStatus ms;
  size_t free = 0;
  long chunks = 0;
  //long iter = 0;
  while (c) { 
    size_t va_c = (void *)c - base;
    lmc_dump_chunk(base, c);
    //iter++;
    //if (iter > 999) {
    //  fprintf(stderr, "dump: loop?\n");
    //  break;
    //}
    free += c->size; 
    chunks++; 
    if (c->next == 0) { c = 0; } else { c = base + c->next;  }
  }
  //printf("===total chunks: %d === free diff: %d =========\n", 
      //chunks, md->total_size - free);
}

int is_va_valid(void *base, size_t va) {
  MemDescriptor *md = base;
  MemChunkDescriptor* c = base + va;
  //printf("va:%d, c: %X, b: %X, e: %X, ts: %d\n", va, c, base, 
  //    base + md->total_size, md->total_size);
  return !(((void *)c < base ) || (base + md->total_size + sizeof(MemDescriptor)) < (void *)c);
}

MemStatus lmc_status(void *base, char *where) {
  MemDescriptor *md = base;
  MemChunkDescriptor* c = md_first_free(base);
  MemStatus ms;
  size_t free = 0;
  size_t largest_chunk = 0;
  long chunks = 0;
  long iter = 0;
  ms.total_mem = md->total_size;
  while (c) { 
    if (!is_va_valid(base, (void *)c - base)) {
      printf("[%s] invalid pointer detected: %d...\n", where, (void *)c - base);
      lmc_dump(base);
      abort();
    }
    if (iter > 99900) {
     printf("n: %d\n", c->next);
    }
    iter++;
    if (iter > 99999) {
      fprintf(stderr, "[%s] status: loop?\n", where);
      //lmc_dump(base);
      exit(0);
      break;
    }
    //printf("size: %d\n", c->size);
    free += c->size; 
    if (c->size > largest_chunk) { largest_chunk = c->size; }
    chunks++; 
    if (c->next == 0) { c = 0; } else { c = base + c->next;  }
  }
  //printf("free: %d\n", free);
  ms.total_free_mem = free;
  ms.free_mem = free > 0 ? free - sizeof(size_t)  : 0;
  ms.largest_chunk = largest_chunk;
  ms.free_chunks = chunks;
  return ms;
  //printf("chunks: %d, free: %d\n", chunks, free);
}

void lmc_show_status(void *base) {
  MemStatus ms = lmc_status(base, "lmc_ss");
  printf("total: %d\n", ms.total_mem);
  printf("chunks: %d, free: %d\n", ms.free_chunks, ms.free_mem);
}

void lmc_init_memory(void *ptr, size_t size) {
  MemDescriptor *md = ptr;
  size_t s = size - sizeof(MemDescriptor);
  // size: enough space for MemDescriptor + MemChunkDescriptor
  md->first_free = sizeof(MemDescriptor);
  md->dummy2 = 0x0;
  md->total_size = s;
  MemChunkDescriptor *c = ptr + sizeof(MemDescriptor);
  c->next = 0;
  c->size = s;
}

size_t lmc_max(size_t a, size_t b) { 
  return a > b ? a : b;
}

size_t __s(char *where, MemStatus ms, size_t mem_before, size_t expected_diff) {
  size_t free = ms.total_free_mem;
  printf("(%s) ", where);
  if (mem_before) { printf("[%d:%d] ", free - mem_before, expected_diff); }
  printf("mem_free: %d, chunks: %d\n", free, ms.free_chunks);
  if (expected_diff && expected_diff != free - mem_before) {
    printf("expected_diff (%d) != diff (%d)\n", expected_diff, 
        free - mem_before);
    abort();
  }
  return free;
}


size_t lmc_valloc(void *base, size_t size) {
  size_t mb = __s("alloc1", lmc_status(base, "lmc_valloc1"), 0, 0);
  MemDescriptor *md = base;
  // MOD by power of 2
  size_t s = lmc_max(size + sizeof(size_t), 
      sizeof(MemChunkDescriptor) + sizeof(size_t));
  // larger than available space?
  MemChunkDescriptor *c = md_first_free(base);
  MemChunkDescriptor *p = NULL;
  if (size == 0) { return 0; }
  while (c && c->size <s ) { 
    p = c;
    if (c->next == 0) {
      c = 0;
      break;
    }
    c = base + c->next; 
  }
  if (!c) {
    //fprintf(stderr, "lmc_valloc: Failed to allocate %d bytes!\n", size);
    return 0;
  }
  MemChunkDescriptor *n = base + c->next;
  size_t r = 0;
  if (c->size - s < sizeof(MemChunkDescriptor)) { s = c->size; }
  printf("lmc_valloc: Target chunk: %d, size: %d\n", (void *)c - base, 
      c->size);
  // -----------------           -------------------
  // | chunk         |   wanted: |                 |
  // -----------------           -------------------
  printf("c->size: %d,  s: %d\n", c->size, s);
  if (c->size == s) {
    if (p) { p->next = c->next; }
    else {md->first_free = c->next; }
    r = (size_t)((void*)c - (void*)base);
  } else {
  // -----------------           -------------------
  // | chunk         |   wanted: |                 |
  // |               |           -------------------
  // -----------------           
    c->size -= s;
    r = (size_t)((void*)c - base) + c->size;
  }
  *(size_t *)(r + base) = s;
  printf("lmc_valloc r: %d, s: %d, r: %d, e: %d\n", r, s, size, r + size + 
      sizeof(size_t));
  __s("alloc2", lmc_status(base, "lmc_va_e"), mb, 0);
  return r + sizeof(size_t);
}

// compact_chunks, 
void lmc_check_coalesce(void *base, size_t va_chunk) {
  MemDescriptor *md = base;
  MemChunkDescriptor *chunk = base + va_chunk;
  size_t c_size = chunk->size;
  size_t va_chunk_p = 0;
  size_t va_c_free_chunk = 0;
  MemChunkDescriptor* c_free_chunk = base + va_c_free_chunk;
  size_t va_previous = 0;
  size_t merge1_chunk = 0;
  int merge1 = 0;
  while (c_free_chunk) {
    va_c_free_chunk = (void *)c_free_chunk - base;
    if (va_c_free_chunk != va_chunk) { 
      if (c_free_chunk->next == va_chunk) { va_chunk_p = va_c_free_chunk; }
      else if (!merge1) {
        // ---------------------- 
        // | a_free_chunk        |
        // ----------------------   <---- if (...)  
        // | chunk               |
        // ----------------------
        if (va_c_free_chunk + c_free_chunk->size == va_chunk) { 
          merge1 = 1;
          merge1_chunk = va_c_free_chunk;
        } else 
        // ---------------------- 
        // | chunk               |
        // ----------------------   <---- if (...) 
        // | a_free_chunk        |
        // ----------------------
        if (va_chunk + c_size == va_c_free_chunk) { 
          size_t mb = __s("coalesce2:1", lmc_status(base, "coalesce2:1"), 0, 0);
          printf("cc2: %d (%d) (p:%d) -> %d (%d) \n", 
              va_c_free_chunk, c_free_chunk->size, va_previous, va_chunk,
              chunk->size);
          //lmc_dump(base);
          chunk->size += c_free_chunk->size;
          //lmc_merge(base, va_chunk, va_c_free_chunk, 0);
          // still needed?
          //if (c_free_chunk->next == va_chunk) { chunk->next = 0; } 
          //else { 
          //  //chunk->next = c_free_chunk->next; 
          //}
          if (chunk->next == va_c_free_chunk) { va_previous = va_chunk; }
          MemChunkDescriptor *p = va_previous ? base + va_previous : 0;
          if (p) { p->next = c_free_chunk->next; }
          printf("cc2:2 %d (%d)\n", va_chunk, chunk->size);
          //lmc_dump(base);
          __s("coalesce2:2", lmc_status(base, "coalesce2:2"), mb, 0);
          break;
        }
      }
      va_previous = va_c_free_chunk;
    }
    va_c_free_chunk = c_free_chunk->next;
    if (va_c_free_chunk == 0) { c_free_chunk = NULL; }
    else { c_free_chunk = base + va_c_free_chunk; }
  }
  // ---------------------- 
  // | a_free_chunk        |
  // ----------------------   <---- if (...)  
  // | chunk               |
  // ----------------------
  if (merge1) {
    MemChunkDescriptor *cd = base + merge1_chunk;
    MemChunkDescriptor *p = va_chunk_p ? base + va_chunk_p : 0;
    MemChunkDescriptor *vacd = va_chunk? base + va_chunk : 0;
    size_t scd = cd ? cd->size : 0;
    size_t svacd = vacd ? vacd->size : 0;
    printf("cc1 %d (%d) (p:%d) -> %d (%d)\n", va_chunk, svacd, 
        va_chunk_p, merge1_chunk, scd);
    printf("md->first_free: %d\n", md->first_free);
    printf("c_size: %d\n", c_size);
    size_t mb = __s("merge1:1", lmc_status(base, "merge1:1"), 0, 0);
    //lmc_dump(base);
    if (p) { p->next = vacd->next; }
    if (md->first_free == va_chunk) { md->first_free = chunk->next; }
    cd->size += c_size;
    printf("cc1:2 %d (%d)\n", merge1_chunk, cd->size);
    __s("merge1:2", lmc_status(base, "merge2:1"), mb, 0);
    //lmc_dump(base);
    //lmc_dump(base);
    //lmc_merge(base, merge1_chunk, va_chunk, 0);
  }
 // printf("AFTER lmc_check_coalesce\n");
  lmc_status(base, "lmc_check_coalesce2");
}


//void d(void *base) {
//  void *used_chunk_p = base + 42133;
//  size_t uc_size = *(size_t *)used_chunk_p;
//  printf("CHECK size: %d\n", uc_size);
//}

void lmc_free(void *base, size_t chunk) {
  size_t mb = __s("free1", lmc_status(base, "lmc_free1"), 0, 0);
  MemDescriptor *md = base;
  size_t va_used_chunk = chunk - sizeof(size_t);
  void *used_chunk_p = base + va_used_chunk;
  MemChunkDescriptor *mcd_used_chunk = used_chunk_p;
  size_t uc_size = *(size_t *)used_chunk_p;
  size_t va_c_free_chunk = 0;
  MemChunkDescriptor* c_free_chunk = base + va_c_free_chunk;
  size_t va_previous = 0;
  size_t va_c_free_end = 0;
  if (!(chunk >= sizeof(MemDescriptor) + sizeof(size_t)) ||
      !is_va_valid(base, chunk)) {
    printf("lmc_free: Invalid pointer: %d\n", chunk);
    return;
  }
  printf("lmc_free: %d, size: %d, e: %d\n", va_used_chunk, uc_size,
    va_used_chunk + uc_size);
  if (uc_size == 0) {
    printf("SIZE is 0!\n");
    lmc_dump(base);
    abort();
  }
  memset(base + chunk, 0xF9, uc_size - sizeof(size_t));
  //long count = 0;
  int freed = 0;
  while (c_free_chunk) {
    va_c_free_chunk = (void *)c_free_chunk - base;
    va_c_free_end = va_c_free_chunk + c_free_chunk->size;
    //count++;
    //if (count > 999) {
    //  printf("free: loop?\n");
    //  lmc_dump(base);
    //  exit(0);
    //  break;
    //}
    /*printf("c_free_chunk: %X, va_c_free_chunk: %d, size: %d\n", 
        c_free_chunk, va_c_free_chunk, c_free_chunk->size);*/
    // ---------------------- 
    // | c_free_chunk        |
    // ----------------------   <---- if (...)  
    // | used_chunk          |
    // ----------------------
    if (va_c_free_end == va_used_chunk) { 
      printf("free: 1\n");
      freed = 1;
      c_free_chunk->size += uc_size;
      //lmc_dump_chunk(base, c_free_chunk);
      lmc_check_coalesce(base, va_c_free_chunk);
      break;
    } else 
    // ---------------------- 
    // | used_chunk          |
    // ----------------------   <---- if (...) 
    // | c_free_chunk        |
    // ----------------------
    if (va_used_chunk + uc_size == va_c_free_chunk) { 
      printf("free: 2: %d -> %d\n", va_c_free_chunk, va_used_chunk);
      freed = 1;
      MemChunkDescriptor *p = base + va_previous;
      mcd_used_chunk->next = c_free_chunk->next;
      //lmc_dump_chunk(base, mcd_used_chunk);
      mcd_used_chunk->size = uc_size + c_free_chunk->size;
      p->next = va_used_chunk;
      lmc_check_coalesce(base, va_used_chunk);
      break;
    }
    if (va_used_chunk >= va_c_free_chunk && va_used_chunk <= va_c_free_end) {
      fprintf(stderr, "Was pointer already freed?\n");
      return;
    }
    va_previous = va_c_free_chunk;
    va_c_free_chunk = c_free_chunk->next;
    if (va_c_free_chunk == 0) { c_free_chunk = NULL; }
    else { c_free_chunk = base + va_c_free_chunk; }
  }
  // ---------------------- 
  // | otherwise allocated |
  // ---------------------- 
  // | used_chunk          |
  // ----------------------   
  // | otherwise allocated |
  // ----------------------
  if (!freed) {
    //d(base);
    printf("free: Not freed: adding to list as a single chunk\n");
    //lmc_dump(base);
    mcd_used_chunk->next = md->first_free;
    mcd_used_chunk->size = uc_size;
    //lmc_dump_chunk(base, mcd_used_chunk);
    md->first_free = va_used_chunk;
    //lmc_dump(base);
    //d(base);
  }
  // printf("after free");
  __s("free2", lmc_status(base, "lmc_free2"), mb, uc_size);
  //d(base);
}


void lmc_realloc(void *base, size_t chunk) {
  // check if enough reserved space, true: resize; otherwise: alloc new and 
  // then free
}

#include <ruby.h>
void *memp = NULL;
static VALUE OutOfMemoryError;

long long_value(VALUE i) { return NUM2LONG(rb_Integer(i)); }
VALUE num2string(long i) { return rb_big2str(rb_int2big(i), 10); }

static VALUE Alloc__new(VALUE klass, VALUE size) {
  size_t s = long_value(size);
  memp = malloc(s);
  printf("memp: %0x, end: %0x\n", memp, memp+s);
  memset(memp, 0xF0, s);
  lmc_init_memory(memp, s);
  return rb_class_new_instance(0, NULL, klass);
}

long sub(long a, long b) {
  long v= a > b ? a - b : 0;
  printf("v: %d\n", v);
  return v;
}

static VALUE Alloc__get(VALUE obj, VALUE size) {
  size_t va = lmc_valloc(memp, long_value(size));
  if (!va) { rb_raise(OutOfMemoryError, "Out of memory"); }
  size_t v = long_value(size);
  //printf("memset: va: 0x%0X, l: 0x%0X\n", memp+va, memp+va+v);
  //memset(memp + va, 0xFB, v);
  memset(memp + va, va, v);
  //if (!memcheck(memp + va, va, v)) { abort(); }
  //printf("memset done\n");
  //d(memp);
  return rb_int2big(va);
}

static VALUE Alloc__dispose(VALUE obj, VALUE adr) {
 // printf("dispose\n");
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

static VALUE Alloc;

void Init_valloc() {
  OutOfMemoryError = rb_define_class("OutOfMemoryError", rb_eStandardError);
  Alloc = rb_define_class("Alloc", rb_cObject);
  rb_define_singleton_method(Alloc, "new", Alloc__new, 1);
  rb_define_method(Alloc, "get", Alloc__get, 1);
  rb_define_method(Alloc, "dispose", Alloc__dispose, 1);
  rb_define_method(Alloc, "dump", Alloc__dump, 0);
  rb_define_method(Alloc, "free_mem", Alloc__free_mem, 0);
  rb_define_method(Alloc, "largest_chunk", Alloc__largest_chunk, 0);
  rb_define_method(Alloc, "free_chunks", Alloc__free_chunks, 0);
}
