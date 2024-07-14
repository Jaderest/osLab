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

// ----------------buddy system----------------
#define PAGE_SHIFT 12
// static size_t buddy_mem_sz = 0;
static buddy_pool_t g_buddy_pool = {};
// static lock_t global_lock = LOCK_INIT();


void buddy_pool_init(buddy_pool_t *pool, void *start, void *end) {
    // 初始化伙伴系统的元数据
    size_t page_num = (end - start) >> PAGE_SHIFT;
    pool->pool_meta_data = (void *)start;
    debug("buddy pool init: start = %p, end = %p, page_num = %d\n", start, end,
        page_num);
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
    } else {
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
    buddy_pool_init(&g_buddy_pool, heap.start, heap.end);
}

MODULE_DEF(pmm) = {
    .init  = pmm_init,
    .alloc = kalloc,
    .free  = kfree,
};