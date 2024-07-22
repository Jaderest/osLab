#include <common.h>
#include "pmm.h"

// static void *pmm_end = NULL;
// static *pmm_start = NULL;

// align
static size_t align_size(size_t size) {
    size_t ret = 8;
    while (ret < size) {
        ret <<= 1;
    }
    ret = (ret > 8) ? ret : 8;
    return ret;
}

// slab
// typedef struct slab {
//     struct slab *next;       // 指向下一个slab
//     object_t *free_objects; // 指向空闲对象链表
//     size_t num_free; // 空闲对象数量
//     lock_t lock; // 用于保护该slab的锁
//     size_t size; // 每个对象的大小
// } slab_t;
// typedef struct cache {
//     slab_t *slabs; // 指向slab链表
//     size_t object_size; // 每个对象的大小
//     lock_t cache_lock; // 用于保护该cache的锁
// } cache_t;

static cache_t caches[MAX_CACHES]; // slab分配器数组
void slab_init() {
    size_t size = 8;
    for (int i = 0; i < MAX_CACHES; i++) {
        caches[i].slabs = NULL;
        caches[i].object_size = size;
        caches[i].cache_lock = LOCK_INIT();
        size <<= 1;
    }
}

static void *kalloc(size_t size) {
    void *ret = NULL;
    size = align_size(size);
    if (size > (1 << MAX_ORDER) * PAGE_SIZE) {
        return NULL;
    } else if (size >= PAGE_SIZE) { 
        // TODO buddy system
    } else {
    }
    return ret;
}

static void kfree(void *ptr) {
    // TODO
    // You can add more .c files to the repo.
}

static void pmm_init() {
    uintptr_t pmsize = (
        (uintptr_t)heap.end
        - (uintptr_t)heap.start
    ); // Area heap = {}; 然后Area里面有两个指针
    printf(
        "Got %d MiB heap: [%p, %p)\n",
        pmsize >> 20, heap.start, heap.end
    );
    slab_init();
    debug("test\n");
}

MODULE_DEF(pmm) = {
    .init  = pmm_init,
    .alloc = kalloc,
    .free  = kfree,
};