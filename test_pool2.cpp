#include "pool2.h"

#include <cstdint>
#include <list>
#include <random>

#include <gtest/gtest.h>

using pool2 = struct pool2;

namespace {

GTEST_TEST(pool2, pool_creation) {
    pool2 *p = pool2_create(1024);
    ASSERT_NE(nullptr, p);
    pool2_destroy(p);
}

GTEST_TEST(pool2, alloc_and_free_ints) {
    struct pool2 *p = pool2_create(80);

    int32_t *i = (int32_t *)pool2_alloc(p, sizeof(int32_t));
    EXPECT_NE(nullptr, i);
    int32_t *j = (int32_t *)pool2_alloc(p, sizeof(int32_t));
    EXPECT_NE(nullptr, j);
    int64_t *k = (int64_t *)pool2_alloc(p, sizeof(int64_t));
    EXPECT_NE(nullptr, k);

    pool2_free(p, i);
    pool2_free(p, k);
    pool2_free(p, j);

    pool2_destroy(p);
}

GTEST_TEST(pool2, reuse) {
    struct pool2 *p = pool2_create(40);
    auto rng{std::minstd_rand()};

    for (int i = 0; i < 2048; ++i) {
        void *ptr = pool2_alloc(p, 16);
        ASSERT_NE(nullptr, ptr);
        pool2_free(p, ptr);
        EXPECT_EQ(40, pool2_available(p));
    }

    pool2_destroy(p);
}

struct some_struct {
    int a, b, c, d;
};

GTEST_TEST(pool2, alloc_and_free_a_struct) {
    struct pool2 *p = pool2_create(80);

    some_struct *i = (some_struct *)pool2_alloc(p, sizeof(some_struct));
    EXPECT_NE(nullptr, i);
    pool2_free(p, i);

    pool2_destroy(p);
}

GTEST_TEST(pool2, allocated_and_available) {
    struct pool2 *p = pool2_create(80);
    EXPECT_EQ(80, pool2_available(p));
    EXPECT_EQ(0, pool2_allocated(p));

    some_struct *i = (some_struct *)pool2_alloc(p, sizeof(some_struct));
    EXPECT_GT(80, pool2_available(p));
    EXPECT_EQ(80 - pool2_available(p), pool2_allocated(p));
    pool2_free(p, i);

    EXPECT_EQ(80, pool2_available(p));
    EXPECT_EQ(0, pool2_allocated(p));

    pool2_destroy(p);
}

GTEST_TEST(pool2, alignment) {
    struct pool2 *p = pool2_create(420);

    int32_t *i = (int32_t *)pool2_alloc(p, sizeof(int32_t));
    EXPECT_NE(nullptr, i);
    EXPECT_EQ(0, (uintptr_t)i % 8);
    pool2_free(p, i);

    char *j = (char *)pool2_alloc(p, sizeof(char));
    EXPECT_NE(nullptr, j);
    EXPECT_EQ(0, (uintptr_t)j % 8);
    pool2_free(p, j);

    double *k = (double *)pool2_alloc(p, sizeof(double));
    EXPECT_NE(nullptr, k);
    EXPECT_EQ(0, (uintptr_t)k % 8);
    pool2_free(p, k);

    some_struct *l = (some_struct *)pool2_alloc(p, sizeof(some_struct));
    EXPECT_NE(nullptr, l);
    EXPECT_EQ(0, (uintptr_t)l % 8);
    pool2_free(p, l);

    pool2_destroy(p);
}

GTEST_TEST(pool2, overhead) {
    constexpr auto expected_overhead = sizeof(unsigned) * 3;
    struct pool2 *p = pool2_create(sizeof(double) + expected_overhead);

    double *i = (double *)pool2_alloc(p, sizeof(double));
    ASSERT_NE(nullptr, i);
    pool2_free(p, i);

    pool2_destroy(p);
}

GTEST_TEST(pool2, block_count) {
    struct pool2 *p = pool2_create(420);

    EXPECT_EQ(1, pool2_free_blocks(p));
    EXPECT_EQ(0, pool2_used_blocks(p));

    int32_t *top = (int32_t *)pool2_alloc(p, sizeof(int32_t));
    EXPECT_EQ(1, pool2_free_blocks(p));
    EXPECT_EQ(1, pool2_used_blocks(p));

    int32_t *mid = (int32_t *)pool2_alloc(p, sizeof(int32_t));
    EXPECT_EQ(1, pool2_free_blocks(p));
    EXPECT_EQ(2, pool2_used_blocks(p));

    int32_t *bot = (int32_t *)pool2_alloc(p, sizeof(int32_t));
    EXPECT_EQ(1, pool2_free_blocks(p));
    EXPECT_EQ(3, pool2_used_blocks(p));

    pool2_free(p, mid);
    EXPECT_EQ(2, pool2_free_blocks(p));
    EXPECT_EQ(2, pool2_used_blocks(p));

    mid = (int32_t *)pool2_alloc(p, sizeof(int32_t));
    EXPECT_EQ(1, pool2_free_blocks(p));
    EXPECT_EQ(3, pool2_used_blocks(p));

    pool2_free(p, mid);
    EXPECT_EQ(2, pool2_free_blocks(p));
    EXPECT_EQ(2, pool2_used_blocks(p));

    pool2_free(p, top);
    EXPECT_EQ(2, pool2_free_blocks(p));
    EXPECT_EQ(1, pool2_used_blocks(p));

    pool2_free(p, bot);
    EXPECT_EQ(1, pool2_free_blocks(p));
    EXPECT_EQ(0, pool2_used_blocks(p));

    pool2_destroy(p);
}

GTEST_TEST(pool2, randoms_allocs) {
    constexpr auto iterations = 2;
    constexpr auto pool2_size = 4 * 1024 * 1024;
    constexpr auto max_item_size = 1024;
    constexpr auto lower_bound = static_cast<int>(0.1 * pool2_size);
    constexpr auto upper_bound = static_cast<int>(0.8 * pool2_size);

    struct pool2 *p = pool2_create(pool2_size);
    auto rng{std::minstd_rand()};
    std::list<void *> allocs{};

    for (uint8_t i = 0; i < iterations; ++i) {
        while (pool2_available(p) > lower_bound) {
            allocs.push_back(pool2_alloc(p, rng() % max_item_size));
        }

        for (auto alloc : allocs) {
            EXPECT_EQ(0, (uintptr_t)alloc % 8);
        }

        while (pool2_available(p) < upper_bound) {
            auto alloc = allocs.begin();
            std::advance(alloc, rng() % allocs.size());
            pool2_free(p, *alloc);
            allocs.erase(alloc);
        }
    }

    for (auto alloc : allocs) {
        pool2_free(p, alloc);
    }

    pool2_destroy(p);
}

} // namespace
