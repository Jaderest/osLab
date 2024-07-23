#include <common.h>
#include <pmm.h>
#ifdef TEST
#include <stdio.h>
#endif

static void *pmm_end = NULL;
static void *pmm_start = NULL;

#define BLOCK_FREE 1
#define BLOCK_ALLOCATED 0

// align
static size_t align_size(size_t size) {
    size_t ret = 8;
    while (ret < size) {
        ret <<= 1;
    }
    ret = (ret > 8) ? ret : 8;
    return ret;
}

void print_pool(buddy_pool_t *pool) {
    for (int i = 0; i <= MAX_ORDER; i++) {
        struct list_head *list = &(pool->free_lists[i].free_list);
        if (list_empty(list)) {
            continue;
        }
        debug("order %d:\n", i); // 即这个链表中的block的order都是i
        buddy_block_t *block = (buddy_block_t *)list->next;
        while (&block->node != list) {
            debug("%p: [%d, %d)\n", block,
                (uintptr_t)block2addr(pool, block),
                (uintptr_t)block2addr(pool, block), (1 << block->order) * PAGE_SIZE);
            block = (buddy_block_t *)block->node.next;
        }
        debug("\n");
    }
}

// ----------------- buddy system -----------------
#define PAGE_SHIFT 12 // 2^12 = 4096
// static size_t buddy_mem_sz = 0;
static buddy_pool_t g_buddy_pool = {};
static lock_t global_lock = LOCK_INIT();

// size is at least 1 page, return the order of size of the size, order is at least 0
static inline size_t buddy_block_order(size_t size) {
    size_t order = 0;
    while ((1 << order) < size) {
        order++;
    }
    // debug("size = %d, order = %d\n", size, order);
    PANIC_ON(size < 1 || order < 0, "size = %d, order = %d", size, order);
    return order;
} // order = log_2(size（页数）)

// 初始化buddy内存池，包括设置元数据、初始化自由列表、标记每个页状态，并将整个内存空间分为一个一个Page
void buddy_pool_init(buddy_pool_t *pool, void *start, void *end) { // 初始化buddy_pool
    //初始化自由列表，标记每个页的状态
    size_t page_num = (end - start) >> PAGE_SHIFT;
    debug("page_num = %d\n", page_num);
    for (int i = 0; i <= MAX_ORDER; i++) {
        init_list_head(&pool->free_lists[i].free_list);
    }
    pool->pool_meta_data = start;
    debug("meta data of buddy pool: [%p, %p)\n", pool->pool_meta_data, pool->pool_meta_data + page_num * sizeof(buddy_block_t));
    memset(pool->pool_meta_data, 0, page_num * sizeof(buddy_block_t));
    PANIC_ON((uintptr_t)pool->pool_meta_data % PAGE_SIZE != 0, "pool_meta_data is not aligned");
    debug("memset done\n");

    start += page_num * sizeof(buddy_block_t); // 从元数据后开始分配
    page_num = (end - start) >> PAGE_SHIFT;
    debug("page_num = %d\n", page_num);
    pool->pool_start_addr = (void *)ALIGN((uintptr_t)start, PAGE_SIZE);
    pool->pool_end_addr = (void *)ALIGN((uintptr_t)end, PAGE_SIZE);
    page_num = (pool->pool_end_addr - pool->pool_start_addr) >> PAGE_SHIFT;
    debug("page_num = %d\n", page_num); // 这里不能assert，因为可能会有一些空洞
    debug("pool_start_addr = %p, pool_end_addr = %p\n", pool->pool_start_addr, pool->pool_end_addr);

    // 整个空间分为一个page，每个page绑定一个buddy_block_t
    // -----------meta_data-----------
    int page_idx;
    // 初始化每一个buddy_block_t
    for (page_idx = 0; page_idx < page_num; page_idx++) {
        buddy_block_t *block = (buddy_block_t *)(pool->pool_meta_data + page_idx * sizeof(buddy_block_t));
        block->order = 0;
        block->free = BLOCK_ALLOCATED;
    }
    // 初始化后，将所有页标记为可用
    for (page_idx = 0; page_idx < page_num; page_idx++) {
        buddy_block_t *block = (buddy_block_t *)(pool->pool_meta_data + page_idx * sizeof(buddy_block_t));
        void *addr = block2addr(pool, block);
        buddy_free(pool, addr);
    }

    print_pool(pool);
}

// 将block转换为地址
void *block2addr(buddy_pool_t *pool, buddy_block_t *block) {
    int index = block - (buddy_block_t *)pool->pool_meta_data;
    void *addr = index * PAGE_SIZE + pool->pool_start_addr;
    return addr;
}

// 将地址转换为block
buddy_block_t *addr2block(buddy_pool_t *pool, void *addr) {
    PANIC_ON(((uintptr_t)addr % PAGE_SIZE), "addr is not aligned");
    int index = (uintptr_t)(addr - pool->pool_start_addr) >> PAGE_SHIFT;
    buddy_block_t *block = (buddy_block_t *)(pool->pool_meta_data + index * sizeof(buddy_block_t));
    return block;
}

// 获取buddy块的伙伴
buddy_block_t *get_buddy_chunk(buddy_pool_t *pool, buddy_block_t *block) {
    uintptr_t addr = (uintptr_t)block2addr(pool, block);
    // 获取伙伴块的地址，具有相同大小，取异或可以得到
    uintptr_t buddy_addr = addr ^ (1 << (block->order + PAGE_SHIFT));
    if (buddy_addr < (uintptr_t)pool->pool_start_addr || buddy_addr >= (uintptr_t)pool->pool_end_addr) {
        return NULL;
    }
    return addr2block(pool, (void *)buddy_addr);
}

// merge the block with its buddy until the order is MAX_ORDER(equal to its buddy)
void buddy_system_merge(buddy_pool_t *pool, buddy_block_t *block) {
    int order = block->order; // 当前块的阶数
    debug("order = %d\n", order);
    while (order < MAX_ORDER) {
        buddy_block_t *buddy = get_buddy_chunk(pool, block); //是把block合成
        if (buddy == NULL || buddy->free == BLOCK_ALLOCATED || buddy->order != order) {
            // NULL || buddy 被占用 || order 不对
            break;
        }
        debug("del block = %p\n", block);
        list_del(&(buddy->node)); // 将buddy所在list删除
        pool->free_lists[order].nr_free--;
        if ((uintptr_t)block > (uintptr_t)buddy) block = buddy; // 这句话是block指向右半块，然后指向统领的buddy起始地址
        order++;
        block->order = order;
        block->free = BLOCK_FREE;
    }
    block->order = order;
    block->free = BLOCK_FREE;
    //TODO: 研究一下list的add和del呢
    debug("add block = %p\n", block);
    list_add(&(block->node), &(pool->free_lists[order].free_list)); // 最后merge留下了那些碎片
    pool->free_lists[order].nr_free++;
}

// 2^12 = 4096
// TODO: 继续完善
void *buddy_alloc(buddy_pool_t *pool, size_t size) {
    lock(&global_lock);
    size = align_size(size);
    debug("get buddy order\n");
    int order = buddy_block_order(size >> PAGE_SHIFT); // 转换为页数
    // buddy_block_t *block = NULL;
    for (int i = order; i <= MAX_ORDER; i++) { // 从order查找可以使用的块

    }

    unlock(&global_lock);
    return NULL;
}

void buddy_free(buddy_pool_t *pool, void *ptr) {
    lock(&global_lock);
    buddy_block_t *block = addr2block(pool, ptr);
    // debug("free block = %p\n", block);
    buddy_system_merge(pool, block);
    unlock(&global_lock);
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

    buddy_pool_init(&g_buddy_pool, pmm_start, pmm_end);
    // print_pool(&g_buddy_pool);
    slab_init();
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