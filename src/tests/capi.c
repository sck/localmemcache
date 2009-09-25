#include <stdio.h>
#include <localmemcache.h>

#define fail_if(t,m) \
  if ((t)) { printf("FAILED: %s\n", (m)); goto test_failed; } \
  else { printf("  - %s\n", (m)); }

#define TEST_START(m) \
  printf("%s\n", (m)); \
  {

#define TEST_END \
  }

int main() {
  lmc_error_t e;
  if (!local_memcache_drop_namespace("capitest", 0, 1, &e)) {
    fprintf(stderr, "drop failed: %s\n", e.error_str);
    return 1;
  }
  local_memcache_t *lmc = local_memcache_create("capitest", 0, 0, 0, &e);
  if (!lmc) {
    fprintf(stderr, "Couldn't create localmemcache: %s\n", e.error_str);
    return 1;
  }
  if (!local_memcache_set(lmc, "foo", 3, "1", 1)) goto failed;

  TEST_START("local_memcache_get_new")
    size_t n_value;
    char *value = local_memcache_get_new(lmc, "foo", 3, &n_value);
    fail_if(!value, "value")
    fail_if(n_value != 1, "n_value");
    fail_if(value[0] != '1', "value[0]");
    free(value);

    char *value2 = local_memcache_get_new(lmc, "unknown", 7, &n_value);
    fail_if(value2, "value2");
  TEST_END

  TEST_START("random_pair")
    char *rk, *rv;
    size_t n_rk, n_rv;
    int r = local_memcache_random_pair_new(lmc, &rk, &n_rk, &rv, &n_rv);
    fail_if(!r, "local_memcache_random_pair_new");
    fail_if(n_rk != 3, "n_rk");
    fail_if(rk[0] != 'f', "rk0");
    fail_if(n_rv != 1, "n_rv");
    fail_if(rv[0] != '1', "rv0");
    free(rk);
    free(rv);
  TEST_END

  if (!local_memcache_delete(lmc, "foo", 3)) goto failed;

  TEST_START("random_pair(empty)")
    char *rk, *rv;
    size_t n_rk, n_rv;
    int r = local_memcache_random_pair_new(lmc, &rk, &n_rk, &rv, &n_rv);
    fail_if(!r, "local_memcache_random_pair_new:empty");
    fail_if(rk, "rk:empty")
    fail_if(rv, "rv:empty")
  TEST_END

  printf("Ok\n");

test_failed:
  if (!local_memcache_free(lmc, &e)) {
    fprintf(stderr, "Failed to release localmemcache: %s\n", e.error_str);
    return 1;
  }


  return 0;

failed:
  fprintf(stderr, "FAILED: %s\n", lmc->error.error_str);
  return 1;

}
