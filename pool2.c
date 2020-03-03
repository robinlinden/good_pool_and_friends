#include "pool2.h"

#include <stdbool.h> // true, false
#include <stdlib.h> // malloc, free, NULL

struct pool2 {
    unsigned size;
};

typedef struct pool2_item_header {
    unsigned size : 30;
    unsigned in_use : 1;
    unsigned first : 1; // no blocks before this
} pool2_item_header;

typedef struct pool2_item_footer {
    unsigned size : 30;
    unsigned last : 1; // no blocks after this
    unsigned : 1; // spare byte if one of the others break
} pool2_item_footer;

#define ALLOCATION_OVERHEAD \
    sizeof(pool2_item_header) + sizeof(pool2_item_footer)

static pool2_item_footer *footer(const pool2_item_header *i) {
    return (void *)((char *)i + i->size - sizeof(pool2_item_footer));
}

static pool2_item_header *first_block(const struct pool2 *pool) {
    return (pool2_item_header *)(pool + 1);
}

static pool2_item_header *next_block(const pool2_item_header *block) {
    return (void *)((char *)block + block->size);
}

static pool2_item_header *prev_block(const pool2_item_header *block) {
    const unsigned prev_block_size = ((pool2_item_footer *)(block - 1))->size;
    return (void *)((char *)block - prev_block_size);
}

struct pool2 *pool2_create(unsigned size) {
    struct pool2 *pool = malloc(size);
    if (!pool) {
        return NULL;
    }

    pool->size = size;

    pool2_item_header *block = first_block(pool);
    block->size = size;
    block->in_use = false;
    block->first = true;

    footer(block)->size = block->size;
    footer(block)->last = true;

    return pool;
}

void pool2_destroy(struct pool2 *pool) {
    free(pool);
}

void *pool2_alloc(struct pool2 *pool, unsigned size) {
    size += (8 - size % 8) % 8;
    size += ALLOCATION_OVERHEAD;

    for (pool2_item_header *block = first_block(pool);
            ;
            block = next_block(block)) {
        if (block->in_use) {
            if (footer(block)->last) {
                return NULL;
            }
            continue;
        }

        if (block->size < size) {
            if (footer(block)->last) {
                return NULL;
            }
            continue;
        }

        block->in_use = true;

        // Is the block big enough to split?
        if (block->size - size > ALLOCATION_OVERHEAD) {
            const unsigned new_size = block->size - size;
            block->size = size;
            footer(block)->size = size;
            footer(block)->last = false;

            pool2_item_header *new_block = next_block(block);
            new_block->size = new_size;
            new_block->in_use = false;
            new_block->first = false;
            footer(new_block)->size = new_size;
            // footer(new_block)->last is inherited from the last block that
            // lived here.
        }

        return block + 1;
    }
}

void pool2_free(struct pool2 *pool, void *ptr) {
    if (!ptr) return;

    pool2_item_header *block = (pool2_item_header *)ptr - 1;
    block->in_use = false;

    if (!footer(block)->last) {
        pool2_item_header *next = next_block(block);
        if (!next->in_use) {
            block->size += next->size;
            footer(block)->size = block->size;
        }
    }

    if (!block->first) {
        pool2_item_header *prev = prev_block(block);
        if (!prev->in_use) {
            prev->size += block->size;
            footer(block)->size = prev->size;
        }
    }
}

unsigned pool2_available(const struct pool2 *pool) {
    unsigned memory = 0;
    for (pool2_item_header *i = first_block(pool);; i = next_block(i)) {
        if (!i->in_use) {
            memory += i->size;
        }
        if (footer(i)->last) {
            return memory;
        }
    }
}

unsigned pool2_allocated(const struct pool2 *pool) {
    unsigned memory = 0;
    for (pool2_item_header *i = first_block(pool);; i = next_block(i)) {
        if (i->in_use) {
            memory += i->size;
        }
        if (footer(i)->last) {
            return memory;
        }
    }
}

unsigned pool2_free_blocks(const struct pool2 *pool) {
    unsigned blocks = 0;
    for (pool2_item_header *i = first_block(pool);; i = next_block(i)) {
        if (!i->in_use) {
            ++blocks;
        }
        if (footer(i)->last) {
            return blocks;
        }
    }
}

unsigned pool2_used_blocks(const struct pool2 *pool) {
    unsigned blocks = 0;
    for (pool2_item_header *i = first_block(pool);; i = next_block(i)) {
        if (i->in_use) {
            ++blocks;
        }
        if (footer(i)->last) {
            return blocks;
        }
    }
}
