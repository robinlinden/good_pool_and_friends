#ifndef GOOD_POOL_H_
#define GOOD_POOL_H_

#include <stddef.h> // size_t

#ifdef __cplusplus
extern "C" {
#endif

struct good_pool;

struct good_pool *pool_create(size_t sz);
void pool_destroy(struct good_pool *pool);

void *pool_alloc(struct good_pool *pool, size_t sz);
void pool_free(struct good_pool *pool, void *ptr);

size_t pool_available(const struct good_pool *pool);
size_t pool_allocated(const struct good_pool *pool);
size_t pool_free_blocks(const struct good_pool *pool);
size_t pool_used_blocks(const struct good_pool *pool);

#ifdef __cplusplus
}
#endif

#endif
