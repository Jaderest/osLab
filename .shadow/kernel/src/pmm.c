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

// ----------------buddy system（Page以上大内存）----------------
// PAGE_SHIFT 是页大小的对数
#define PAGE_SHIFT 12
// static size_t buddy_mem_sz = 0;
static buddy_pool_t g_buddy_pool = {};
static lock_t global_lock = LOCK_INIT();

void debug_pool(buddy_pool_t *pool) {
    for (int i = 0; i <= MAX_ORDER; i++) {
        struct list_head *list = &(pool->free_lists[i].free_list);
        if (list_empty(list)) {
            continue;
        }
        debug("order %d: ", i);
        buddy_block_t *block = (buddy_block_t *)list->next;
        while (&block->node != list) {
            debug("%p :", block);
            debug("range: [%p, %p), ", block2addr(pool, block),
                block2addr(pool, block) + (1 << block->order) * PAGE_SIZE);
            block = (buddy_block_t *)block->node.next;
        }
        debug("\n");
    }
}

void buddy_pool_init(buddy_pool_t *pool, void *start, void *end) {
    // free lists
    size_t page_num = (end - start) >> PAGE_SHIFT;
    pool->pool_meta_data = (void *)start;
    debug("buddy pool init: start = %p, end = %p, page_num = %d\n", start, end,
        page_num);
    for (int i = 0; i <= MAX_ORDER; i++) {
        init_list_head(&pool->free_lists[i].free_list);
    }
    debug("meta data of buddy pool: [%p, %p)\n", pool->pool_meta_data,
        pool->pool_meta_data + page_num * sizeof(buddy_block_t));
    // buddy blocks

    // memset()
    start += page_num * sizeof(buddy_block_t); // meta data
    page_num -= page_num * sizeof(buddy_block_t) >> PAGE_SHIFT;
    pool->pool_start_addr = (void*)ALIGN((uintptr_t)start, PAGE_SIZE);
    pool->pool_end_addr = end;
    page_num = (pool->pool_end_addr - pool->pool_start_addr) >> PAGE_SHIFT;
    debug("memory that can be allocated: [%p, %p)\n", pool->pool_start_addr,
        pool->pool_end_addr);

    // 将整个内存空间分为一个个page，每个page绑定一个buddy_block_t
    int page_idx;
    for (page_idx = 0; page_idx < page_num; page_idx++) {
        buddy_block_t *block = (buddy_block_t *)pool->pool_meta_data + sizeof(buddy_block_t) * page_idx;
        block->order = 0;
        block->free = 0;
    }

    for (page_idx = 0; page_idx < page_num; page_idx++) {
        buddy_block_t *block = (buddy_block_t *)pool->pool_meta_data + sizeof(buddy_block_t) * page_idx;
        void *addr = block2addr(pool, block); // block -> addr
        buddy_free(pool, addr); // 要将所有的page都放到free list中
    }
    // debug_pool(pool);
}

// split the block until the order is equal to target_order
buddy_block_t *buddy_system_split(buddy_pool_t *pool, buddy_block_t *block, int target_order) {
    buddy_block_t *ret = NULL;
    int order = block->order;
    while (order > 0 && order > target_order) {
        order--;
        ret = split2buddies(pool, ret, order);
    }
    return ret;
}

buddy_block_t *get_buddy_chunk(buddy_pool_t *pool, buddy_block_t *block) {
    uintptr_t addr = (uintptr_t)block2addr(pool, block);
    uintptr_t buddy_addr = addr ^ (1UL << (block->order + PAGE_SHIFT));
    if (buddy_addr < (uintptr_t)pool->pool_start_addr || buddy_addr + (1UL << (block->order + PAGE_SHIFT)) >= (uintptr_t)pool->pool_end_addr) {
        return NULL;
    }
    return addr2block(pool, (void *)buddy_addr);
}

buddy_block_t *split2buddies(buddy_pool_t *pool, buddy_block_t *old, int new_order) {
    panic_on(old->order <= 0, "split2buddies: order <= 0");
    uintptr_t left_addr = (uintptr_t)block2addr(pool, old);
    uintptr_t right_addr = left_addr + (1 << (new_order + PAGE_SHIFT));
    buddy_block_t *left = addr2block(pool, (void *)left_addr);
    buddy_block_t *right = addr2block(pool, (void *)right_addr);
    left->order = new_order;
    right->order = new_order;
    left->free = 0;
    right->free = 1;
    list_add((struct list_head *)right, &(pool->free_lists[new_order].free_list));
    pool->free_lists[new_order].nr_free++;
    return left;
}

void *block2addr(buddy_pool_t *pool, buddy_block_t *block) {
    int index = ((void *)block - pool->pool_meta_data) / sizeof(buddy_block_t);
    void *addr = index * PAGE_SIZE + pool->pool_start_addr;
    return addr;
}

buddy_block_t *addr2block(buddy_pool_t *pool, void *addr) {
    panic_on(((uintptr_t)addr % PAGE_SIZE), "addr is supposed to aligned to page");
    int index = (uintptr_t)(addr - pool->pool_start_addr) >> PAGE_SHIFT;
    return (buddy_block_t *)(pool->pool_meta_data + index * sizeof(buddy_block_t));
}

void buddy_system_merge(buddy_pool_t *pool, buddy_block_t *block) {
    int order = block->order;
    while (order < MAX_ORDER) { //TODO: 卡死在这了
        buddy_block_t *buddy = get_buddy_chunk(pool, block);
        if (buddy == NULL || buddy->free == 0 || buddy->order != order) {
            break;
        }
        list_del((struct list_head *)buddy);
        pool->free_lists[order].nr_free--;
        if ((uintptr_t)buddy < (uintptr_t)block) {
            block = buddy;
        }
        order++;
        block->order = order;
        block->free = 1;
    }
    block->order = order;
    block->free = 1;
    list_add(&(block->node), &(pool->free_lists[order].free_list));
    pool->free_lists[order].nr_free++;
}

void buddy_free(buddy_pool_t *pool, void *addr) {
    lock(&global_lock);
    buddy_block_t *block = addr2block(pool, addr);
    buddy_system_merge(pool, block);
    unlock(&global_lock);
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
    buddy_pool_init(&g_buddy_pool, heap.start, heap.end);
}

MODULE_DEF(pmm) = {
    .init  = pmm_init,
    .alloc = kalloc,
    .free  = kfree,
};