/*
 * Copyright (c) 2009, Sven C. Koehler
 */

#ifndef _LMC_ERROR_H_
#define _LMC_ERROR_H_

#define LMC_OK 1
#define LMC_ERROR 0

typedef struct {
  char error_str[1024];
  char error_type[1024];
  int error_number;
} lmc_error_t;

#define STD_OUT_OF_MEMORY_ERROR(ctx) \
    lmc_handle_error_with_err_string((ctx), "Out of memory error", \
        "OutOfMemoryError", 0, e)

#define LMC_MEMORY_POOL_FULL(ctx) \
    lmc_handle_error_with_err_string((ctx), "Memory pool full", \
        "MemoryPoolFull", 0, e)

int lmc_handle_error(int check, const char *ctx, const char *error_type, 
    char *handle, lmc_error_t* e);
int lmc_handle_error_with_err_string(const char *ctx, const char *error_msg,
    const char *error_type, char *handle, lmc_error_t* e);
#endif
