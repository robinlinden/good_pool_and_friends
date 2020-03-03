#ifndef POOL2_H_
#define POOL2_H_

#ifdef __cplusplus
extern "C" {
#endif

struct pool2;

struct pool2 *pool2_create(unsigned size);
void pool2_destroy(struct pool2 *pool);

void *pool2_alloc(struct pool2 *pool, unsigned size);
void pool2_free(struct pool2 *pool, void *ptr);

unsigned pool2_available(const struct pool2 *pool);
unsigned pool2_allocated(const struct pool2 *pool);
unsigned pool2_free_blocks(const struct pool2 *pool);
unsigned pool2_used_blocks(const struct pool2 *pool);

#ifdef __cplusplus
}
#endif

#endif POOL2_H_
