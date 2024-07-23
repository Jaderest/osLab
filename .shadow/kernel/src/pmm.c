#include <common.h>
#include <pmm.h>
#ifdef TEST
#include <stdio.h>
#endif

static void *pmm_end = NULL;
static void *pmm_start = NULL;

// align
static size_t align_size(size_t size) {
    size_t ret = 8;
    while (ret < size) {
        ret <<= 1;
    }
    ret = (ret > 8) ? ret : 8;
    return ret;
}

// ----------------- buddy system -----------------
#define PAGE_SHIFT 12
// static size_t buddy_mem_sz = 0;
static buddy_pool_t g_buddy_pool = {};
static lock_t global_lock = LOCK_INIT();

// size is at least 1 page, return the order of size of the size, order is at least 0
static inline size_t buddy_block_order(size_t size) {
    size_t order = 0;
    while ((1 << order) < size) {
        order++;
    }
    debug("size = %d, order = %d\n", size, order);
    return order;
}

// 2^12 = 4096
void *buddy_alloc(buddy_pool_t *pool, size_t size) {
    lock(&global_lock);
    size = align_size(size);
    buddy_block_order(size >> PAGE_SHIFT); // 转换为页数

    unlock(&global_lock);
    return NULL;
}





// ---------------- slab allocator ----------------
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
        // debug("caches[%d].object_size = %d\n", i, caches[i].object_size);
    } // 8 16 32 64 128 256 512 1024 2048
    debug("slab_init done\n");
}

// static cache_t *find_cache(size_t size) {
//     for (int i = 0; i < MAX_CACHES; i++) { // 找到最适合的cache页面
//         if (caches[i].object_size >= size) {
//             return &caches[i];
//         }
//     }
//     return NULL;
// }

// static slab_t *slab_alloc_in_cache(cache_t *cache) {
//     /**
//      * 从 buddy system中分配一个 page 作为 slab 起始点
//      * 将 slab 用元数据填充， 并将剩余部分分割成 object
//      * obj 被链接为一个链表
//      * 将 slab 加入 cache 的 slab 链表
//      */
//     return NULL;
// }

static void *kalloc(size_t size) {
    void *ret = NULL;
    size = align_size(size);
    // if (size > (1 << MAX_ORDER) * PAGE_SIZE) {
    //     return NULL;
    // } else if (size >= PAGE_SIZE) { 
    //     // TODO buddy system
    // } else {
    // }
    ret = buddy_alloc(&g_buddy_pool, size);
    PANIC_ON(ret == NULL, "Failed to allocate %d bytes", size);
    return ret;
}

static void kfree(void *ptr) {
    // TODO
    // You can add more .c files to the repo.
}

#ifndef TEST
static void pmm_init() {
    uintptr_t pmsize = (
        (uintptr_t)heap.end
        - (uintptr_t)heap.start
    ); // Area heap = {}; 然后Area里面有两个指针
    printf(
        "Got %d MiB heap: [%p, %p)\n",
        pmsize >> 20, heap.start, heap.end
    );
    pmm_start = heap.start;
    pmm_end = heap.end;

    slab_init();
    debug("test\n");
}
#else
// 测试框架中的pmm_init
#define PMM_SIZE (128 << 20)
Area heap = {};
static void pmm_init() {
    char *ptr = malloc(PMM_SIZE);
    if (ptr == NULL) {
        // PANIC("Failed to allocate %d MiB heap", PMM_SIZE >> 20);
    }
    heap.start = ptr;
    heap.end = ptr + PMM_SIZE;
    heap.start = (void *)ALIGN((uintptr_t)heap.start, PAGE_SIZE);
    heap.end = (void *)ALIGN((uintptr_t)heap.end, PAGE_SIZE);
    uintptr_t pmsize = (uintptr_t)heap.end - (uintptr_t)heap.start;
    pmm_start = heap.start;
    pmm_end = heap.end;
    printf("Got %ld MiB heap: [%p, %p)\n", pmsize >> 20, heap.start, heap.end);
}
#endif

MODULE_DEF(pmm) = {
    .init  = pmm_init,
    .alloc = kalloc,
    .free  = kfree,
};