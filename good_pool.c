#include "good_pool.h"

#include <stdlib.h>

struct good_pool_item {
    size_t sz;
    struct good_pool_item *next;
};

struct good_pool {
    void *addr;

    struct good_pool_item *used;
    struct good_pool_item *free;
};

static struct good_pool_item *first_fit(struct good_pool *p, size_t sz) {
    for (struct good_pool_item *i = p->free; i != NULL; i = i->next) {
        if (i->sz >= sz) {
            return i;
        }
    }

    return NULL;
}

static void list_remove(
        struct good_pool_item *head,
        struct good_pool_item *remove) {
    while (head->next && head->next != remove) head = head->next;
    if (head->next == remove) head->next = remove->next;
}

static void pool_remove_used(
        struct good_pool *p,
        struct good_pool_item *i) {
    if (p->used == i) {
        p->used = p->used->next;
    } else {
        list_remove(p->used, i);
    }
}

static void pool_remove_free(
        struct good_pool *p,
        struct good_pool_item *i) {
    if (p->free == i) {
        p->free = p->free->next;
    } else {
        list_remove(p->free, i);
    }
}

static struct good_pool_item *list_find(
        struct good_pool_item *head,
        struct good_pool_item *needle) {
    if (head == needle) return head;
    while (head->next && head->next != needle) head = head->next;
    return head->next;
}

static void *to_external_ptr(struct good_pool_item *i) {
    return ((char *)i) + sizeof(struct good_pool_item);
}

static struct good_pool_item *to_pool_ptr(void *ptr) {
    return (void *)((char *)ptr - sizeof(struct good_pool_item));
}

static void pool_coalesce(struct good_pool *p) {
    if (!p->free || !p->free->next) return;

    if (p->free > p->free->next) {
        struct good_pool_item *tmp = p->free->next;
        p->free->next = tmp->next;
        tmp->next = p->free;
        p->free = tmp;
    }

    for (struct good_pool_item *i = p->free;
            i->next != NULL && i->next->next != NULL;
            i = i->next) {
        if (i->next > i->next->next) {
            struct good_pool_item *tmp = i->next->next;
            i->next->next = tmp->next;
            tmp->next = i->next;
            i->next = tmp;
        }
    }

    for (struct good_pool_item *i = p->free; i->next != NULL; i = i->next) {
        if ((uintptr_t)i + i->sz == (uintptr_t)i->next) {
            i->sz += i->next->sz;
            i->next = i->next->next;
            if (i->next == NULL) return;
        }
    }
}

// TODO(robinlinden): Let the pool live in the one allocation? Probably.
struct good_pool *pool_create(size_t sz) {
    struct good_pool *p = malloc(sizeof(*p));
    p->addr = malloc(sz);
    p->used = NULL;

    p->free = p->addr;
    p->free->sz = sz;
    p->free->next = NULL;
    return p;
}

void pool_destroy(struct good_pool *p) {
    free(p->addr);
    free(p);
}

void *pool_alloc(struct good_pool *p, size_t sz) {
    sz += (8 - sz % 8) % 8;
    size_t actual_sz = sz + sizeof(struct good_pool_item);
    struct good_pool_item *i = first_fit(p, actual_sz);
    if (!i) return NULL;

    if (p->free->next) {
        pool_remove_free(p, i);
    } else {
        p->free = NULL;
    }

    if (i->sz >= actual_sz + sizeof(struct good_pool_item)) {
        struct good_pool_item *remainder = (void *)((char *)i + actual_sz);
        remainder->sz = i->sz - actual_sz;
        i->sz = actual_sz;
        remainder->next = p->free;
        p->free = remainder;
    }

    i->next = p->used;
    p->used = i;

    return to_external_ptr(i);
}

void pool_free(struct good_pool *p, void* ptr) {
    if (!ptr) return;

    struct good_pool_item *i = list_find(p->used, to_pool_ptr(ptr));
    pool_remove_used(p, i);

    i->next = p->free;
    p->free = i;

    pool_coalesce(p);
}

size_t pool_available(const struct good_pool *p) {
    size_t available = 0;

    for (struct good_pool_item *i = p->free; i != NULL; i = i->next) {
        available += i->sz;
    }

    return available;
}

size_t pool_allocated(const struct good_pool *p) {
    size_t allocated = 0;

    for (struct good_pool_item *i = p->used; i != NULL; i = i->next) {
        allocated += i->sz;
    }

    return allocated;
}

size_t pool_free_blocks(const struct good_pool *p) {
    size_t blocks = 0;

    for (struct good_pool_item *i = p->free; i != NULL; i = i->next) {
        ++blocks;
    }

    return blocks;
}

size_t pool_used_blocks(const struct good_pool *p) {
    size_t blocks = 0;

    for (struct good_pool_item *i = p->used; i != NULL; i = i->next) {
        ++blocks;
    }

    return blocks;
}
