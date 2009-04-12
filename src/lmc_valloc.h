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
} lmc_mem_status_t;

typedef struct {
   int op_id;
   size_t p1;
   size_t p2;
} lmc_log_descriptor_t;

typedef struct {
  size_t first_free;
  size_t dummy2;
  size_t total_size;
  size_t magic;
  size_t va_hash;
  int locked;
  size_t version;
  lmc_log_descriptor_t log;
} lmc_mem_descriptor_t;

typedef struct {
  size_t next;
  size_t size;
} lmc_mem_chunk_descriptor_t;


size_t lmc_valloc(void *base, size_t size);
void lmc_free(void *base, size_t chunk);
lmc_mem_status_t lmc_status(void *base, char *where);
int is_lmc_already_initialized(void *base);
void lmc_init_memory(void *ptr, size_t size);

int lmc_um_mark_allocated(void *base, char *bf, size_t va);
char *lmc_um_new_mem_usage_bitmap(void *base);
int lmc_um_find_leaks(void *base, char *bf);

lmc_log_descriptor_t *lmc_log_op(void *base, int opid);
void lmc_log_finish(void *base);

#endif
