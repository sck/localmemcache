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

#include "lmc_valloc.h"
#include "lmc_lock.h"
#include "lmc_common.h"

#undef lmc_valloc 

lmc_mem_chunk_descriptor_t *md_first_free(void *base) {
  lmc_mem_descriptor_t *md = base;
  return md->first_free == 0 ? 0 : base + md->first_free;
}

void lmc_dump_chunk(void *base, lmc_mem_chunk_descriptor_t* c) {
  size_t va_c = (void *)c - base;
  printf("chunk %zd:\n"
      "  start: %zd\n"
      "  end  : %zd\n"
      "  size : %zd\n"
      "  next : %zd\n"
      "  ------------------------\n"
      , va_c, va_c, va_c + c->size, c->size, c->next);
}

void lmc_dump_chunk_brief(char *who, void *base, lmc_mem_chunk_descriptor_t* c) {
  if (!c) { return; }
  size_t va_c = (void *)c - base;
  printf("[%s] chunk %zd:\n", who, va_c);
}


void lmc_dump(void *base) {
  lmc_mem_chunk_descriptor_t* c = md_first_free(base);
  size_t free = 0;
  long chunks = 0;
  while (c) { 
    lmc_dump_chunk(base, c);
    free += c->size; 
    chunks++; 
    if (c->next == 0) { c = 0; } else { c = base + c->next;  }
  }
}

int lmc_is_va_valid(void *base, size_t va) {
  lmc_mem_descriptor_t *md = base;
  lmc_mem_chunk_descriptor_t* c = base + va;
  return !(((void *)c < base ) || 
      (base + md->total_size + sizeof(lmc_mem_descriptor_t)) < (void *)c);
}

lmc_mem_status_t lmc_status(void *base, char *where) {
  lmc_mem_descriptor_t *md = base;
  lmc_mem_chunk_descriptor_t* c = md_first_free(base);
  lmc_mem_status_t ms;
  size_t free = 0;
  size_t largest_chunk = 0;
  long chunks = 0;
  ms.total_mem = md->total_size;
  while (c) { 
    if (!lmc_is_va_valid(base, (void *)c - base)) {
      printf("[localmemcache] [%s] invalid pointer detected: %zd...\n", where, 
          (void *)c - base);
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
  lmc_mem_status_t ms = lmc_status(base, "lmc_ss");
  printf("total: %zu\n", ms.total_mem);
  printf("chunks: %zu, free: %zu\n", ms.free_chunks, ms.free_mem);
}

int is_lmc_already_initialized(void *base) {
  lmc_mem_descriptor_t *md = base;
  if (md->magic == 0xF00D) {
#ifdef LMC_DEBUG_ALLOC
    printf("memory already initialized, skipping...\n");
#endif
    return 1;
  }
  return 0;
}

void lmc_init_memory(void *ptr, size_t size) {
  lmc_mem_descriptor_t *md = ptr;
  size_t s = size - sizeof(lmc_mem_descriptor_t);
  md->first_free = sizeof(lmc_mem_descriptor_t);
  md->magic = 0xF00D;
  md->version = LMC_DB_VERSION;
  md->locked = 1;
  md->total_size = s;
  lmc_mem_chunk_descriptor_t *c = ptr + sizeof(lmc_mem_descriptor_t);
  c->next = 0;
  c->size = s;
}

size_t lmc_get_db_version(void *ptr) {
  lmc_mem_descriptor_t *md = ptr;
  return md->version;
}

size_t lmc_max(size_t a, size_t b) { 
  return a > b ? a : b;
}

size_t __s(char *where, lmc_mem_status_t ms, size_t mem_before, size_t expected_diff) {
  size_t free = ms.total_free_mem;
  printf("(%s) ", where);
  if (mem_before) { printf("[%zd:%zd] ", free - mem_before, expected_diff); }
  printf("mem_free: %zu, chunks: %zu\n", free, ms.free_chunks);
  if (expected_diff && expected_diff != free - mem_before) {
    printf("expected_diff (%zu) != diff (%zd)\n", expected_diff, 
        free - mem_before);
    abort();
  }
  return free;
}

size_t lmc_valloc(void *base, size_t size) {
  lmc_mem_descriptor_t *md = base;
  // consider: make size divisible by power of 2
  size_t s = lmc_max(size + sizeof(size_t), 
      sizeof(lmc_mem_chunk_descriptor_t) + sizeof(size_t));
  lmc_mem_chunk_descriptor_t *c = md_first_free(base);
  lmc_mem_chunk_descriptor_t *p = NULL;
  if (size == 0) { return 0; }
  while (c && c->size < s ) { 
    p = c;
    if (c->next == 0) {
      c = 0;
      break;
    }
    c = base + c->next; 
  }
  if (!c) {
    return 0;
  }
  size_t r = 0;
  if (c->size - s < sizeof(lmc_mem_chunk_descriptor_t)) { s = c->size; }
  // -----------------           -------------------
  // | chunk         |   wanted: |                 |
  // -----------------           -------------------
  if (c->size == s) {
    if (p) { p->next = c->next; }
    else {md->first_free = c->next; }
    LMC_TEST_CRASH
    r = (size_t)((void*)c - (void*)base);
  } else {
  // -----------------           -------------------
  // | chunk         |   wanted: |                 |
  // |               |           -------------------
  // -----------------           
    c->size -= s;
    LMC_TEST_CRASH
    r = (size_t)((void*)c - base) + c->size;
  }
  *(size_t *)(r + base) = s;
  return r + sizeof(size_t);
}

void lmc_compact_free_chunks(void *base, size_t va_chunk) {
  lmc_mem_descriptor_t *md = base;
  lmc_mem_chunk_descriptor_t *chunk = base + va_chunk;
  size_t c_size = chunk->size;
  size_t va_chunk_p = 0;
  size_t va_c_free_chunk = 0;
  lmc_mem_chunk_descriptor_t* c_free_chunk = base + va_c_free_chunk;
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
          LMC_TEST_CRASH
          if (chunk->next == va_c_free_chunk) { va_previous = va_chunk; }
          LMC_TEST_CRASH
          lmc_mem_chunk_descriptor_t *p = va_previous ? base + va_previous : 0;
          LMC_TEST_CRASH
          if (p) { p->next = c_free_chunk->next; }
          LMC_TEST_CRASH
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
    lmc_mem_chunk_descriptor_t *cd = base + merge1_chunk;
    lmc_mem_chunk_descriptor_t *p = va_chunk_p ? base + va_chunk_p : 0;
    lmc_mem_chunk_descriptor_t *vacd = va_chunk? base + va_chunk : 0;
    LMC_TEST_CRASH
    if (p) { p->next = vacd->next; }
    LMC_TEST_CRASH
    if (md->first_free == va_chunk) { md->first_free = chunk->next; }
    LMC_TEST_CRASH
    cd->size += c_size;
  }
}

void __lmc_free(void *base, size_t va_used_chunk, size_t uc_size) {
  void *used_chunk_p = base + va_used_chunk;
  lmc_mem_descriptor_t *md = base;
  lmc_mem_chunk_descriptor_t *mcd_used_chunk = used_chunk_p;
  size_t va_c_free_chunk = 0;
  lmc_mem_chunk_descriptor_t* c_free_chunk = base + va_c_free_chunk;
  size_t va_previous = 0;
  size_t va_c_free_end = 0;
#ifdef LMC_DEBUG_ALLOC
  if (uc_size == 0) {
    printf("SIZE is 0!\n");
    lmc_dump(base);
    abort();
  }
  memset(base + va_used_chunk - sizeof(size_t), 0xF9, uc_size - sizeof(size_t));
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
      LMC_TEST_CRASH
      c_free_chunk->size += uc_size;
      LMC_TEST_CRASH
      lmc_compact_free_chunks(base, va_c_free_chunk);
      break;
    } else 
    // ---------------------- 
    // | used_chunk          |
    // ----------------------   <---- if (...) 
    // | c_free_chunk        |
    // ----------------------
    if (va_used_chunk + uc_size == va_c_free_chunk) { 
      freed = 1;
      lmc_mem_chunk_descriptor_t *p = base + va_previous;
      mcd_used_chunk->next = c_free_chunk->next;
      mcd_used_chunk->size = uc_size + c_free_chunk->size;
      p->next = va_used_chunk;
      lmc_compact_free_chunks(base, va_used_chunk);
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

void lmc_free(void *base, size_t chunk) {
#ifdef LMC_DEBUG_ALLOC
  size_t mb = __s("free1", lmc_status(base, "lmc_free1"), 0, 0);
#endif
  if (chunk == 0) { return; }
  if (!(chunk >= sizeof(lmc_mem_descriptor_t) + sizeof(size_t)) ||
      !lmc_is_va_valid(base, chunk)) {
    fprintf(stderr, "[localmemcache] lmc_free: Invalid pointer: %zd\n", chunk);
    return;
  }
  size_t va_used_chunk = chunk - sizeof(size_t);
  void *used_chunk_p = base + va_used_chunk;
  size_t uc_size = *(size_t *)used_chunk_p;
  __lmc_free(base, va_used_chunk, uc_size);
}

int lmc_um_getbit(char *bf, int i) {
  bf += i / 8; return (*bf & (1 << (i % 8))) != 0;
}

void lmc_um_setbit(char *bf, int i, int v) {
  bf += i / 8;
  if (v) *bf |= 1 << (i % 8);    
  else *bf &= ~(1 << (i % 8));  
}

int lmc_um_find_leaks(void *base, char *bf) {
  lmc_mem_descriptor_t *md = base;
  size_t i;
  int gap = 0;
  size_t gs = 0;
  size_t m;
  size_t gap_count = 0;
  size_t space = 0;
  memset(&m, 0xFF, sizeof(m));
  for (i = 0; i < md->total_size; ++i) { 
    if (!gap) {
      size_t *b = (void *)bf + i / 8;
      while (*b == m && i < md->total_size - sizeof(size_t)) { 
        i += sizeof(size_t) * 8; b++; 
      }
    }
    if (lmc_um_getbit(bf, i) == 0) {
      if (!gap) {
        gs = i;
        gap = 1;
        gap_count++;
      }
    } else {
      if (gap) {
        gap = 0;
        space += i - gs;
        __lmc_free(base, gs, i - gs);
      }
    }
  }
  if (gap) {
    gap = 0;
    space += i - gs;
    __lmc_free(base, gs, i - gs);
  }
  return 1;
}

int lmc_um_check_unmarked(void *base, char *bf, size_t va, size_t size) {
  size_t i;
  size_t n;
  memset(&n, 0x0, sizeof(n));
  size_t end = va + size;
  for (i = va; i < va + size; ++i) { 
    size_t *b = (void *)bf + i / 8;
    while (*b == n && i < end - sizeof(size_t)) { 
      i += sizeof(size_t) * 8; b++; 
    }
    if (lmc_um_getbit(bf, i) != 0) { return 0; }
  }
  return 1;
}

int lmc_um_mark(void *base, char *bf, size_t va, size_t size) {
  size_t i;
  lmc_mem_descriptor_t *md = base;
  if ((va > sizeof(lmc_mem_descriptor_t)) &&
      (!lmc_is_va_valid(base, va) || !lmc_is_va_valid(base, va + size))) {
    printf("[localmemcache] Error: VA start out of range: "
        "va: %zd - %zd max %zd!\n", va, va + size, md->total_size);
    return 0;
  }
  if (!lmc_um_check_unmarked(base, bf, va, size)) return 0;
  for (i = va; i < va + size; ++i) { 
    if (i % 8 == 0) {
      size_t b_start = i / 8;
      size_t b_end = (va + size - 1) / 8;
      if (b_start != b_end) {
        memset(bf + b_start, 0xFF, b_end - b_start);
        i += (b_end * 8) - va;
      }
    }
    lmc_um_setbit(bf, i, 1); 
  }
  return 1;
}

int lmc_um_mark_allocated(void *base, char *bf, size_t va) {
  size_t real_va = va - sizeof(size_t);
  size_t s = *(size_t *)(base + real_va);
  return lmc_um_mark(base, bf, real_va, s);
}

char *lmc_um_new_mem_usage_bitmap(void *base) {
  lmc_mem_descriptor_t *md = base;
  size_t ts = ((md->total_size + 7) / 8);
  char *bf = calloc(1, ts);
  size_t va = md->first_free;
  if (!lmc_um_mark(base, bf, 0, sizeof(lmc_mem_descriptor_t))) goto failed;
  lmc_mem_chunk_descriptor_t *c;
  while (va) { 
    c = base + va;
    if (!lmc_um_mark(base, bf, va, c->size)) goto failed;
    va = c->next;
  }
  return bf;

failed:
  free(bf);
  return 0;
}

lmc_log_descriptor_t *lmc_log_op(void *base, int opid) {
  lmc_mem_descriptor_t *md = base;
  lmc_log_descriptor_t *l = &md->log;
  l->op_id  = opid;
  l->p1 = l->p2 = 0x0;
  return l;
}

void lmc_log_finish(void *base) {
  lmc_mem_descriptor_t *md = base;
  lmc_log_descriptor_t *l = &md->log;
  l->op_id = 0;
  l->p1 = l->p2 = 0x0;
}
