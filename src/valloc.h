/*
 * Copyright (c) 2009, Sven C. Koehler
 */

#ifndef _LMC_VALLOC_H_INCLUDED_
#define _LMC_VALLOC_H_INCLUDED_
#undef LMC_DEBUG_ALLOC
typedef struct {
  size_t free_chunks;
  size_t total_mem;
  size_t total_free_mem;
  size_t free_mem;
  size_t largest_chunk;
} mem_status_t;

typedef struct {
  size_t first_free;
  size_t dummy2;
  size_t total_size;
  size_t magic;
  size_t va_hash;
} mem_descriptor_t;


size_t lmc_valloc(void *base, size_t size);
void lmc_free(void *base, size_t chunk);
mem_status_t lmc_status(void *base, char *where);
int is_lmc_already_initialized(void *base);
void lmc_init_memory(void *ptr, size_t size);
#endif
