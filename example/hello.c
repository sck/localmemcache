#include <stdio.h>
#include <localmemcache.h>

int main() {
  lmc_error_t e;
  local_memcache_t *lmc = local_memcache_create("viewcounters", 0, &e);
  if (!lmc) {
    fprintf(stderr, "Couldn't create localmemcache: %s\n", e.error_str);
    return 1;
  }
  if (!local_memcache_set(lmc, "foo", 3, "1", 1)) goto failed;
  size_t n_value;
  char *value = local_memcache_get_new(lmc, "foo", 3, &n_value);
  if (!value) goto failed;
  free(value);
  if (!local_memcache_delete(lmc, "foo", 3)) goto failed;
  if (!local_memcache_free(lmc, &e)) {
    fprintf(stderr, "Failed to release localmemcache: %s\n", e.error_str);
    return 1;
  }

  return 0;

failed:
  fprintf(stderr, "%s\n", lmc->error.error_str);
  return 1;

}
