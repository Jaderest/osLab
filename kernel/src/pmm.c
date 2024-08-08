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
    size_t ret = 1;
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
        buddy_block_t *block = (buddy_block_t *)list->next;
        while (&block->node != list) {
            block = (buddy_block_t *)block->node.next;
        }
    }
}

// ----------------- buddy system -----------------
#define PAGE_SHIFT 12 // 2^12 = 4096
static size_t buddy_mem_sz = 0;
static buddy_pool_t g_buddy_pool = {};
static spinlock_t global_lock = spinlock_init("global_lock");

// size is at least 1 page, return the order of size of the size, order is at least 0
static inline size_t buddy_block_order(size_t size) {
    size_t order = 0;
    while ((1 << order) < size) {
        order++;
    }
#ifdef TEST
    PANIC_ON(size < 1 || order < 0, "size = %ld, order = %ld", size, order);
#else
    PANIC_ON(size < 1 || order < 0, "size = %d, order = %d", size, order);
#endif
    return order;
} // order = log_2(size（页数）)

// 初始化buddy内存池，包括设置元数据、初始化自由列表、标记每个页状态，并将整个内存空间分为一个一个Page
void buddy_pool_init(buddy_pool_t *pool, void *start, void *end) { // 初始化buddy_pool
    //初始化自由列表，标记每个页的状态
    size_t page_num = (end - start) >> PAGE_SHIFT;
    pool->pool_meta_data = start;
    for (int i = 0; i <= MAX_ORDER; i++) {
        init_list_head(&pool->free_lists[i].free_list); // 初始化链表，每个order（也即层数）放一个freelist来存放空闲的block
    }
    log("meta data of buddy pool: [%p, %p)\n", pool->pool_meta_data, pool->pool_meta_data + page_num * sizeof(buddy_block_t));
    memset(pool->pool_meta_data, 0, page_num * sizeof(buddy_block_t));
    PANIC_ON((uintptr_t)pool->pool_meta_data % PAGE_SIZE != 0, "pool_meta_data is not aligned");

    start += page_num * sizeof(buddy_block_t); // 从元数据后开始分配
    page_num = (end - start) >> PAGE_SHIFT;
    pool->pool_start_addr = (void *)ALIGN((uintptr_t)start, PAGE_SIZE);
    pool->pool_end_addr = end;
    page_num = (pool->pool_end_addr - pool->pool_start_addr) >> PAGE_SHIFT;

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
        buddy_free(pool, addr); // 通过这个创建链表
    }

    // print_pool(pool);
}

buddy_block_t *buddy_system_split(buddy_pool_t *pool, buddy_block_t *block, int target_order) {
    buddy_block_t *ret = block;
    // PANIC_ON(block->free == BLOCK_ALLOCATED, "block is not free"); 传进来前设置的就是ALLOCATED
    int order = block->order;
    while (order > 0 && order >= target_order + 1) {
        order--;
        ret = split2buddies(pool, ret, order); //还没有达到target的时候就不断再分，然后交给这个函数处理链表关系
    }
#ifdef TEST
    PANIC_ON(ret->order != target_order, "ret->order = %ld, target_order = %d", ret->order, target_order);
#else
    PANIC_ON(ret->order != target_order, "ret->order = %d, target_order = %d", ret->order, target_order);
#endif
    return ret;
}

buddy_block_t *split2buddies(buddy_pool_t *pool, buddy_block_t *old, int new_order) {
    PANIC_ON(new_order < 0 || new_order > MAX_ORDER, "new_order = %d", new_order);
    uintptr_t left_addr = (uintptr_t)block2addr(pool, old); // 左半块的地址
    uintptr_t right_addr = left_addr + (1 << (new_order + PAGE_SHIFT)); // 右半块的地址
    buddy_block_t *left = addr2block(pool, (void *)left_addr); // 左半块的block
    buddy_block_t *right = addr2block(pool, (void *)right_addr); // 右半块的block
    left->order = new_order;
    right->order = new_order;
    left->free = BLOCK_ALLOCATED;
    right->free = BLOCK_FREE;
    list_add((struct list_head *)right, &(pool->free_lists[new_order].free_list)); // 将右半块加入到空闲链表
    pool->free_lists[new_order].nr_free++;
    return left; //! 那原来的block呢，在buddy_alloc中已经处理了，free list已经放好了
}

// 将block转换为地址(映射到分配区里面)
void *block2addr(buddy_pool_t *pool, buddy_block_t *block) {
    // int index = block - (buddy_block_t *)pool->pool_meta_data;
    int index = ((void *)block - pool->pool_meta_data) / sizeof(buddy_block_t);
    void *addr = index * PAGE_SIZE + pool->pool_start_addr;
    return addr;
}

// 将地址转换为block(从page分配过来)
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
    if (buddy_addr < (uintptr_t)pool->pool_start_addr || buddy_addr + (1 << (block->order + PAGE_SHIFT)) >= (uintptr_t)pool->pool_end_addr) {
        return NULL;
    }
    return addr2block(pool, (void *)buddy_addr);
}

// merge the block with its buddy until the order is MAX_ORDER(equal to its buddy)
void buddy_system_merge(buddy_pool_t *pool, buddy_block_t *block) {
    int order = block->order; // 当前块的阶数
    while (order < MAX_ORDER) {
        buddy_block_t *buddy = get_buddy_chunk(pool, block); //是把block合成
        if (buddy == NULL || buddy->free == BLOCK_ALLOCATED || buddy->order != order) {
            // NULL || buddy 被占用 || order 不对
            break;
        }
        list_del(&(buddy->node)); // 将buddy所在list删除
        pool->free_lists[order].nr_free--;
        if ((uintptr_t)block > (uintptr_t)buddy) block = buddy; // 这句话是block指向右半块，然后指向统领的buddy起始地址
        order++;
        block->order = order;
        block->free = BLOCK_FREE;
    }
    block->order = order;
    block->free = BLOCK_FREE;
    list_add(&(block->node), &(pool->free_lists[order].free_list)); // 最后merge留下了那些碎片
    pool->free_lists[order].nr_free++;
}

// 2^12 = 4096
void *buddy_alloc(buddy_pool_t *pool, size_t size) {
    _spin_lock(&global_lock);
    size = align_size(size);
    int order = buddy_block_order(size >> PAGE_SHIFT); // 转换为页数
    buddy_block_t *block = NULL;
    for (int i = order; i <= MAX_ORDER; i++) { // 从order查找可以使用的块
        struct list_head *list = &(pool->free_lists[i].free_list);
        if (!list_empty(list)) {
            block = (buddy_block_t *)list->next;
            // list_del((struct list_head *)block);
            list_del((struct list_head *)block); // 此处已经移除了
            pool->free_lists[i].nr_free--;
            block->free = BLOCK_ALLOCATED; // 标记为已经分配
            block = buddy_system_split(pool, block, order); 
            break;
        }
    }
    if (block == NULL) {
        _spin_unlock(&global_lock);
        return NULL;
    }

    _spin_unlock(&global_lock);
    return block2addr(pool, block);
}

void buddy_free(buddy_pool_t *pool, void *ptr) {
    _spin_lock(&global_lock);
    buddy_block_t *block = addr2block(pool, ptr);
    buddy_system_merge(pool, block);
    _spin_unlock(&global_lock);
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

// 从 2^3==8 到 2^11
static cache_t g_caches[MAX_CACHES]; // 全局slab分配器数组
void slab_init() {
    size_t size = 8;
    for (int i = 0; i < MAX_CACHES; i++) {
        g_caches[i].slabs = NULL;
        g_caches[i].object_size = size;
        g_caches[i].cache_lock = spinlock_init("cache_lock");
        size *= 2;
    } // 每个缓存对应一个固定对象大小8 16 32 64 128 256 512 1024 2048（一直到PAGE_SIZE/2）
}

// 寻找可以分配给object的cache（object_size >= size)
// 这个是必存在的
static cache_t *find_cache(size_t size) {
    for (int i = 0; i < MAX_CACHES; i++) { // 找到最适合的cache页面
        if (g_caches[i].object_size >= size) {
            return &g_caches[i];
        }
    }
    return NULL;
}

static slab_t *allocate_slab(cache_t *cache) {
    // 从 buddy system 申请一个page
    // 前面一小段是给 slab 元数据占据的，然后计算出 object 区域的起始地址
    // 将 slab 加入 cache（它是一个可增长的链表
    uintptr_t slab_addr = (uintptr_t)buddy_alloc(&g_buddy_pool, PAGE_SIZE); // 作为起始指针
    buddy_block_t *block = addr2block(&g_buddy_pool, (void *)slab_addr);
    block->slab = 1; // 表明这个block是slab分配器的一部分
    PANIC_ON(slab_addr % PAGE_SIZE != 0, "slab align error");

    slab_t *new_slab = (slab_t *)slab_addr;
    // 对齐对象区域起始地址
    slab_addr += sizeof(slab_t);
    slab_addr = ALIGN(slab_addr, cache->object_size); // 这里对齐靠不靠谱啊
    PANIC_ON(slab_addr % cache->object_size != 0, "slab align error");
    // 初始化 slab 元数据
    size_t num_objects = (PAGE_SIZE - (slab_addr - (uintptr_t)new_slab)) / cache->object_size;
    PANIC_ON(num_objects == 0, "num_objects = 0");
    // 初始化对象链表
    object_t *obj = (object_t *)slab_addr; // obj起始地址
    new_slab->free_objects = obj;
    new_slab->num_free = num_objects;
    new_slab->size = cache->object_size;
    new_slab->lock = spinlock_init("slab_lock");
    // 填充对象并链接链表
    for (int i = 0; i < num_objects - 1; i++) {
        obj->next = (object_t *)((uintptr_t)obj + new_slab->size);
        obj = obj->next; //obj 一个一个往后推
        PANIC_ON((uintptr_t)obj % cache->object_size != 0, "obj align error");
        PANIC_ON((uintptr_t)obj + new_slab->size > (uintptr_t)new_slab + PAGE_SIZE, "obj out of range");
    }
    obj->next = NULL;

    return new_slab;
}

void *slab_alloc(size_t size) {
#ifdef TEST
    log("slab_alloc: %ld\n", size);
#else
    // log("slab_alloc: %d\n", size);
#endif
    if (size == 0 || size >= PAGE_SIZE) { //用户的非法请求
        return NULL;
    }

    cache_t *cache = find_cache(size); //得到相应的一个cache
    if (cache == NULL) {
        PANIC("slab alloc"); // 这里应该panic吗
        return NULL;
    }

    slab_t *slab = cache->slabs;
    while (slab != NULL) {
        _spin_lock(&slab->lock);
        if (slab->free_objects > 0) {
            object_t *obj = slab->free_objects;
            slab->free_objects = obj->next; // 一个一个往后推
            slab->num_free--;
            _spin_unlock(&slab->lock);
            return obj; //! 所以这里返回的是obj的指针，obj需要对齐
        } else {
            _spin_unlock(&slab->lock);
            slab = slab->next;
        }
    }
    // 此时当前slab没有空闲位置
    PANIC_ON(slab != NULL, "Find slab Error!");

    // 这里的 cache 的 size 是对齐的
    slab = allocate_slab(cache); // 申请一个新的slab
    _spin_lock(&slab->lock);
    object_t *obj = slab->free_objects;
    slab->free_objects = obj->next; 
    slab->num_free--;
    _spin_unlock(&slab->lock);
    _spin_lock(&cache->cache_lock);
    slab->next = cache->slabs; // 头插
    cache->slabs = slab;
    _spin_unlock(&cache->cache_lock);

    return obj;
}

void slab_free(void *ptr) {
    if (ptr == NULL) {
        return;
    }
    // 通过指针找到slab
    object_t *obj = (object_t *)ptr;
    uintptr_t slab_addr = (uintptr_t)ptr & ~(PAGE_SIZE - 1);
    slab_t *slab = (slab_t *)slab_addr;

    _spin_lock(&slab->lock);
    obj->next = slab->free_objects; //嗷这里分配没重置它
    slab->free_objects = obj;
    slab->num_free++;
    _spin_unlock(&slab->lock);
}

static void *kalloc(size_t size) {
    void *ret = NULL;
    size = align_size(size);
    if (size > (1 << MAX_ORDER) * PAGE_SIZE) {
        return NULL;
    } else if (size >= PAGE_SIZE) { 
        ret = buddy_alloc(&g_buddy_pool, size);
    } else {
        ret = slab_alloc(size);
    }
    // ret = buddy_alloc(&g_buddy_pool, size);
    // PANIC_ON(ret == NULL, "Failed to allocate %d bytes", size);
    return ret;
}

static void kfree(void *ptr) {
    // You can add more .c files to the repo.
    void *page = (void *)((uintptr_t)ptr & ~(PAGE_SIZE - 1));
    buddy_block_t *block = addr2block(&g_buddy_pool, page);
    if (block->slab) {
        slab_free(ptr);
    } else {
        buddy_free(&g_buddy_pool, ptr);
    }
}

#ifndef TEST
static void pmm_init() {
    heap.start = (void *)ALIGN((uintptr_t)heap.start, PAGE_SIZE);
    heap.end = (void *)ALIGN((uintptr_t)heap.end, PAGE_SIZE);
    uintptr_t pmsize = (
        (uintptr_t)heap.end
        - (uintptr_t)heap.start
    ); // Area heap = {}; 然后Area里面有两个指针
    pmm_start = heap.start;
    pmm_end = heap.end;
    buddy_mem_sz = pmsize;
    printf(
        "Got %d MiB heap\n", pmsize >> 20);

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
    buddy_mem_sz = pmsize;
    buddy_pool_init(&g_buddy_pool, pmm_start, pmm_end);
    slab_init();
    printf("Got %ld MiB heap: [%p, %p)\n", pmsize >> 20, heap.start, heap.end);
}
#endif

MODULE_DEF(pmm) = {
    .init  = pmm_init,
    .alloc = kalloc,
    .free  = kfree,
};