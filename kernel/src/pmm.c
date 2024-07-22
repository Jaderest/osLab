#include <common.h>
#include "pmm.h"


// static void *pmm_end = NULL;
// static *pmm_start = NULL;

// align
static size_t align_size(size_t size) {
    size_t ret = 16;
    while (ret < size) {
        ret <<= 1;
    }
    ret = (ret > 16) ? ret : 16;
    return ret;
}


static void *kalloc(size_t size) {
    void *ret = NULL;
    size = align_size(size);
    if (size > (1 << MAX_ORDER) * PAGE_SIZE) {
        return NULL;
    } else if (size >= PAGE_SIZE) { // buddy system
        // ret = buddy_alloc(&g_buddy_pool, size);
        // panic_on(((uintptr_t)ret >= (uintptr_t)g_buddy_pool.pool_end_addr),
        //     "buddy_alloc: out of memory");
    } else { // slab allocator
        // ret = slab_alloc(size);
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
    debug("test\n");
}

MODULE_DEF(pmm) = {
    .init  = pmm_init,
    .alloc = kalloc,
    .free  = kfree,
};