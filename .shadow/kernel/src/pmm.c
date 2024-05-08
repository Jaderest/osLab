#include <common.h>

#define MAX_CPU 8

// 一个空闲块的结构
typedef struct freeblock_t {
    struct freeblock_t *next;
    int size;
    int magic;
} freeblock_t;

// 每个CPU设计一个page
typedef struct page_nodes_t { // 页中的块们
    struct page_nodes_t *next;
    int size;
} page_nodes_t;

typedef struct pagelist_t { // 每个cpu有一个的
    page_nodes_t *first; // CPU所属的页链表
    // lock_t lock;
} pagelist_t;
// 设计：每个CPU在自己pages中分配释放时，只需获得对应pagelist_t的锁，不会轻易竞争

// spin_lock heap_lock;
freeblock_t *head;
pagelist_t cpu_pagelist[MAX_CPU];

#define PAGE_SIZE (64 * 1024)

// 我们通过空闲链表分配页（每个cpu），分配页：这个cpu第一次kalloc  or  当前所需内存块大小的page没有，所以分配一个new page
void kallocpage() {
    //TODO：每个cpu分配页（作为缓存），每个cpu有许多页，通过链表来管理
    // 获得堆区大锁，空闲链表分配页，释放页
    // free如何实现？
}

void *kallocFast(size_t size) {
    //TODO: 分配小内存，page中直接解决，不行就新page
    int cpu_order = cpu_current();
    int cpu_nums = cpu_count();
    printf("cpu_order: %d\n", cpu_order);
    printf("cpu_nums: %d\n", cpu_nums);

    return NULL;
}

void *kallocHugeMemory(size_t size) {
    //TODO: 分配大内存，大于page的内存

    return NULL;
}

static void *kalloc(size_t size) {
    if (size == 0) {
        return NULL;
    } else if (size < 16) { // 最小单元16字节
        size = 16;
    } else if (size > PAGE_SIZE) { //16KiB大内存
        if (size > 16 * 1024 * 1024) {
            return NULL;
        } // 16MiB
        // 大内存分配
        size_t align = 16;
        while (align < size) {
            align *= 2;
        }
        size = align;
        void *ret = kallocHugeMemory(size);
        return ret;
    } else { // 16 ~ 16KiB
        size_t align = 16;
        while (align < size) {
            align *= 2;
        }
        size = align;
        printf("size: %d\n", size);
    }

    void *ret = kallocFast(size);
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
}

MODULE_DEF(pmm) = {
    .init  = pmm_init,
    .alloc = kalloc,
    .free  = kfree,
};