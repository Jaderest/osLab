#ifndef _PMM_H
#define _PMM_H

#include <common.h>
#include "list.h"
#include "thread.h"

#define PAGE_SIZE 4096
#define MIB (1024 * 1024)

// slab allocator
typedef struct object {
    struct object *next;
} object_t;

typedef struct slab {
    struct slab *next;       // 指向下一个slab
    object_t *free_objects; // 指向空闲对象链表
    size_t num_free; // 空闲对象数量
    lock_t lock; // 用于保护该slab的锁
    size_t size; // 每个对象的大小
} slab_t;

// slab分配器(链表数组)
typedef struct cache {
    slab_t *slabs; // 指向slab链表
    size_t object_size; // 每个对象的大小
    lock_t cache_lock; // 用于保护该cache的锁
} cache_t;

#endif