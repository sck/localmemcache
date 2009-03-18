/*
 * Copyright (c) 2009, Sven C. Koehler
 */

#ifndef _LMC_COMMON_H_INCLUDED_
#define _LMC_COMMON_H_INCLUDED_

extern int lmc_test_crash_enabled;
#ifdef DO_TEST_CRASH
#define LMC_TEST_CRASH lmc_test_crash(__FILE__, __LINE__, __FUNCTION__);
#else
#define LMC_TEST_CRASH
#endif

#ifdef DO_TEST_ALLOC_FAILURE
#define lmc_valloc(base, s) \
    lmc_test_valloc_fail(__FILE__, __LINE__, __FUNCTION__, base, s)
#endif


void lmc_test_crash(const char* file, int line, const char *function);
size_t lmc_test_valloc_fail(const char *file, int line, const char *function,
    void *base, size_t s);

#endif

