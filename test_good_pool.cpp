#include "good_pool.h"

#include <cstdint>

#include <gtest/gtest.h>

namespace {

GTEST_TEST(good_pool, pool_creation) {
    struct good_pool *p = pool_create(1024);
    ASSERT_NE(nullptr, p);
    pool_destroy(p);
}

GTEST_TEST(good_pool, alloc_and_free_ints) {
    struct good_pool *p = pool_create(80);

    int32_t *i = (int32_t *)pool_alloc(p, sizeof(int32_t));
    EXPECT_NE(nullptr, i);
    int32_t *j = (int32_t *)pool_alloc(p, sizeof(int32_t));
    EXPECT_NE(nullptr, j);
    int32_t *k = (int32_t *)pool_alloc(p, sizeof(int32_t));
    EXPECT_NE(nullptr, k);
    int32_t *l = (int32_t *)pool_alloc(p, sizeof(int32_t));
    EXPECT_NE(nullptr, l);

    pool_free(p, l);
    pool_free(p, i);
    pool_free(p, k);
    pool_free(p, j);

    pool_destroy(p);
}

GTEST_TEST(DISABLED_good_pool, reuse) {
    struct good_pool *p = pool_create(40);

    for (int i = 0; i < 2048; ++i) {
        void *ptr = pool_alloc(p, rand() % 16);
        ASSERT_NE(nullptr, ptr);
        pool_free(p, ptr);
    }

    pool_destroy(p);
}

struct some_struct {
    int a, b, c, d;
};

GTEST_TEST(good_pool, alloc_and_free_a_struct) {
    struct good_pool *p = pool_create(80);

    some_struct *i = (some_struct *)pool_alloc(p, sizeof(some_struct));
    EXPECT_NE(nullptr, i);
    pool_free(p, i);

    pool_destroy(p);
}

GTEST_TEST(good_pool, allocated_and_available) {
    struct good_pool *p = pool_create(80);
    EXPECT_EQ(80, pool_available(p));
    EXPECT_EQ(0, pool_allocated(p));

    some_struct *i = (some_struct *)pool_alloc(p, sizeof(some_struct));
    EXPECT_GT(80, pool_available(p));
    EXPECT_EQ(80 - pool_available(p), pool_allocated(p));
    pool_free(p, i);

    EXPECT_EQ(80, pool_available(p));
    EXPECT_EQ(0, pool_allocated(p));

    pool_destroy(p);
}

GTEST_TEST(good_pool, DISABLED_alignment) {
    struct good_pool *p = pool_create(420);

    int32_t *i = (int32_t *)pool_alloc(p, sizeof(int32_t));
    EXPECT_NE(nullptr, i);
    EXPECT_EQ(0, (uintptr_t)i % 8);
    pool_free(p, i);

    char *j = (char *)pool_alloc(p, sizeof(char));
    EXPECT_NE(nullptr, j);
    EXPECT_EQ(0, (uintptr_t)j % 8);
    pool_free(p, j);

    double *k = (double *)pool_alloc(p, sizeof(double));
    EXPECT_NE(nullptr, k);
    EXPECT_EQ(0, (uintptr_t)k % 8);
    pool_free(p, k);

    some_struct *l = (some_struct *)pool_alloc(p, sizeof(some_struct));
    EXPECT_NE(nullptr, l);
    EXPECT_EQ(0, (uintptr_t)l % 8);
    pool_free(p, l);

    pool_destroy(p);
}

GTEST_TEST(DISABLED_good_pool, block_count) {
    struct good_pool *p = pool_create(420);

    EXPECT_EQ(1, pool_free_blocks(p));
    EXPECT_EQ(0, pool_used_blocks(p));

    int32_t *top = (int32_t *)pool_alloc(p, sizeof(int32_t));
    EXPECT_EQ(1, pool_free_blocks(p));
    EXPECT_EQ(1, pool_used_blocks(p));

    int32_t *mid = (int32_t *)pool_alloc(p, sizeof(int32_t));
    EXPECT_EQ(1, pool_free_blocks(p));
    EXPECT_EQ(2, pool_used_blocks(p));

    int32_t *bot = (int32_t *)pool_alloc(p, sizeof(int32_t));
    EXPECT_EQ(1, pool_free_blocks(p));
    EXPECT_EQ(3, pool_used_blocks(p));

    pool_free(p, mid);
    EXPECT_EQ(2, pool_free_blocks(p));
    EXPECT_EQ(2, pool_used_blocks(p));

    mid = (int32_t *)pool_alloc(p, sizeof(int32_t));
    EXPECT_EQ(1, pool_free_blocks(p));
    EXPECT_EQ(3, pool_used_blocks(p));

    pool_free(p, mid);
    EXPECT_EQ(2, pool_free_blocks(p));
    EXPECT_EQ(2, pool_used_blocks(p));

    pool_free(p, top);
    EXPECT_EQ(2, pool_free_blocks(p));
    EXPECT_EQ(1, pool_used_blocks(p));

    pool_free(p, bot);
    EXPECT_EQ(1, pool_free_blocks(p));
    EXPECT_EQ(0, pool_used_blocks(p));

    pool_destroy(p);
}

} // namespace
