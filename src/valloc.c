/*
 * Copyright (c) 2009, Sven C. Koehler
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

#include "valloc.h"

#include "lock.h"

typedef struct {
  size_t next;
  size_t size;
} mem_chunk_descriptor_t;

mem_chunk_descriptor_t *md_first_free(void *base) {
  mem_descriptor_t *md = base;
  return md->first_free == 0 ? 0 : base + md->first_free;
}

void lmc_dump_chunk(void *base, mem_chunk_descriptor_t* c) {
  size_t va_c = (void *)c - base;
  printf("chunk %zd:\n"
      "  start: %zd\n"
      "  end  : %zd\n"
      "  size : %zd\n"
      "  next : %zd\n"
      "  ------------------------\n"
      , va_c, va_c, va_c + c->size, c->size, c->next);
}

void lmc_dump_chunk_brief(char *who, void *base, mem_chunk_descriptor_t* c) {
  if (!c) { return; }
  size_t va_c = (void *)c - base;
  printf("[%s] chunk %zd:\n", who, va_c);
}


void lmc_dump(void *base) {
  mem_chunk_descriptor_t* c = md_first_free(base);
  size_t free = 0;
  long chunks = 0;
  while (c) { 
    lmc_dump_chunk(base, c);
    free += c->size; 
    chunks++; 
    if (c->next == 0) { c = 0; } else { c = base + c->next;  }
  }
}

int is_va_valid(void *base, size_t va) {
  mem_descriptor_t *md = base;
  mem_chunk_descriptor_t* c = base + va;
  return !(((void *)c < base ) || (base + md->total_size + sizeof(mem_descriptor_t)) < (void *)c);
}

mem_status_t lmc_status(void *base, char *where) {
  mem_descriptor_t *md = base;
  mem_chunk_descriptor_t* c = md_first_free(base);
  mem_status_t ms;
  size_t free = 0;
  size_t largest_chunk = 0;
  long chunks = 0;
  ms.total_mem = md->total_size;
  while (c) { 
    if (!is_va_valid(base, (void *)c - base)) {
      printf("[%s] invalid pointer detected: %ld...\n", where, (void *)c - base);
      lmc_dump(base);
      abort();
    }
    free += c->size; 
    if (c->size > largest_chunk) { largest_chunk = c->size; }
    chunks++; 
    if (c->next == 0) { c = 0; } else { c = base + c->next;  }
  }
  ms.total_free_mem = free;
  ms.free_mem = free > 0 ? free - sizeof(size_t)  : 0;
  ms.largest_chunk = largest_chunk;
  ms.free_chunks = chunks;
  return ms;
}

void lmc_show_status(void *base) {
  mem_status_t ms = lmc_status(base, "lmc_ss");
  printf("total: %zu\n", ms.total_mem);
  printf("chunks: %zu, free: %zu\n", ms.free_chunks, ms.free_mem);
}

int is_lmc_already_initialized(void *base) {
  mem_descriptor_t *md = base;
  if (md->magic == 0xF00D) {
    printf("memory already initialized, skipping...\n");
    return 1;
  }
  return 0;
}

void lmc_init_memory(void *ptr, size_t size) {
  mem_descriptor_t *md = ptr;
  size_t s = size - sizeof(mem_descriptor_t);
  // size: enough space for mem_descriptor_t + mem_chunk_descriptor_t
  md->first_free = sizeof(mem_descriptor_t);
  md->magic = 0xF00D;
  md->locked = 0;
  md->total_size = s;
  mem_chunk_descriptor_t *c = ptr + sizeof(mem_descriptor_t);
  c->next = 0;
  c->size = s;
}

size_t lmc_max(size_t a, size_t b) { 
  return a > b ? a : b;
}

size_t __s(char *where, mem_status_t ms, size_t mem_before, size_t expected_diff) {
  size_t free = ms.total_free_mem;
  printf("(%s) ", where);
  if (mem_before) { printf("[%ld:%zd] ", free - mem_before, expected_diff); }
  printf("mem_free: %zu, chunks: %zu\n", free, ms.free_chunks);
  if (expected_diff && expected_diff != free - mem_before) {
    printf("expected_diff (%zu) != diff (%ld)\n", expected_diff, 
        free - mem_before);
    abort();
  }
  return free;
}

size_t lmc_valloc(void *base, size_t size) {
  mem_descriptor_t *md = base;
  // MOD by power of 2
  size_t s = lmc_max(size + sizeof(size_t), 
      sizeof(mem_chunk_descriptor_t) + sizeof(size_t));
  // larger than available space?
  mem_chunk_descriptor_t *c = md_first_free(base);
  mem_chunk_descriptor_t *p = NULL;
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
  size_t r = 0;
  if (c->size - s < sizeof(mem_chunk_descriptor_t)) { s = c->size; }
  // -----------------           -------------------
  // | chunk         |   wanted: |                 |
  // -----------------           -------------------
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
  return r + sizeof(size_t);
}

// compact_chunks, 
void lmc_check_coalesce(void *base, size_t va_chunk) {
  mem_descriptor_t *md = base;
  mem_chunk_descriptor_t *chunk = base + va_chunk;
  size_t c_size = chunk->size;
  size_t va_chunk_p = 0;
  size_t va_c_free_chunk = 0;
  mem_chunk_descriptor_t* c_free_chunk = base + va_c_free_chunk;
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
          chunk->size += c_free_chunk->size;
          if (chunk->next == va_c_free_chunk) { va_previous = va_chunk; }
          mem_chunk_descriptor_t *p = va_previous ? base + va_previous : 0;
          if (p) { p->next = c_free_chunk->next; }
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
    mem_chunk_descriptor_t *cd = base + merge1_chunk;
    mem_chunk_descriptor_t *p = va_chunk_p ? base + va_chunk_p : 0;
    mem_chunk_descriptor_t *vacd = va_chunk? base + va_chunk : 0;
    if (p) { p->next = vacd->next; }
    if (md->first_free == va_chunk) { md->first_free = chunk->next; }
    cd->size += c_size;
  }
}


void lmc_free(void *base, size_t chunk) {
#ifdef LMC_DEBUG_ALLOC
  size_t mb = __s("free1", lmc_status(base, "lmc_free1"), 0, 0);
#endif
  if (chunk == 0) { return; }
  mem_descriptor_t *md = base;
  size_t va_used_chunk = chunk - sizeof(size_t);
  void *used_chunk_p = base + va_used_chunk;
  mem_chunk_descriptor_t *mcd_used_chunk = used_chunk_p;
  size_t uc_size = *(size_t *)used_chunk_p;
  size_t va_c_free_chunk = 0;
  mem_chunk_descriptor_t* c_free_chunk = base + va_c_free_chunk;
  size_t va_previous = 0;
  size_t va_c_free_end = 0;
  if (!(chunk >= sizeof(mem_descriptor_t) + sizeof(size_t)) ||
      !is_va_valid(base, chunk)) {
    printf("lmc_free: Invalid pointer: %zd\n", chunk);
    return;
  }
#ifdef LMC_DEBUG_ALLOC
  if (uc_size == 0) {
    printf("SIZE is 0!\n");
    lmc_dump(base);
    abort();
  }
  memset(base + chunk, 0xF9, uc_size - sizeof(size_t));
#endif
  int freed = 0;
  while (c_free_chunk) {
    va_c_free_chunk = (void *)c_free_chunk - base;
    va_c_free_end = va_c_free_chunk + c_free_chunk->size;
    // ---------------------- 
    // | c_free_chunk        |
    // ----------------------   <---- if (...)  
    // | used_chunk          |
    // ----------------------
    if (va_c_free_end == va_used_chunk) { 
      freed = 1;
      c_free_chunk->size += uc_size;
      lmc_check_coalesce(base, va_c_free_chunk);
      break;
    } else 
    // ---------------------- 
    // | used_chunk          |
    // ----------------------   <---- if (...) 
    // | c_free_chunk        |
    // ----------------------
    if (va_used_chunk + uc_size == va_c_free_chunk) { 
      freed = 1;
      mem_chunk_descriptor_t *p = base + va_previous;
      mcd_used_chunk->next = c_free_chunk->next;
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
    mcd_used_chunk->next = md->first_free;
    mcd_used_chunk->size = uc_size;
    md->first_free = va_used_chunk;
  }
#ifdef LMC_DEBUG_ALLOC
  __s("free2", lmc_status(base, "lmc_free2"), mb, uc_size);
#endif
}

void lmc_realloc(void *base, size_t chunk) {
  // check if enough reserved space, true: resize; otherwise: alloc new and 
  // then free
}
