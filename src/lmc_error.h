/*
 * Copyright (c) 2009, Sven C. Koehler
 */

#ifndef _LMC_ERROR_H_
#define _LMC_ERROR_H_

#define LMC_OK 1
#define LMC_ERROR 0

typedef struct {
  char error_str[1024];
  int error_number;
} lmc_error_t;

int lmc_handle_error(int check, const char *ctx, lmc_error_t* e);
int lmc_handle_error_with_err_string(const char *ctx, const char* error_msg,
    lmc_error_t* e);
#endif
