#include <common.h>

#define MAX_CPU 8

#ifdef TEST
#include <am.h>
void putch(char ch) {
    putchar(ch);
}
#endif

#define DEBUG
#ifdef DEBUG
    #define debug(...) printf(__VA_ARGS__)
#else
    #define debug(...)
#endif


//TODO: 自旋锁
#define UNLOCKED 0
#define LOCKED 1
// typedef struct lock_t {
//     int flag;
// } lock_t;
// void lock_init(int *lock) {
//     atomic_xchg(lock, LOCKED);
// }
void lock(int *lock) {
    while(atomic_xchg(lock, LOCKED) == LOCKED) {/*spin*/};
}
void unlock(int *lock) {
    panic_on(atomic_xchg(lock, UNLOCKED) != LOCKED, "unlock failed");
}

#define PAGE_SIZE (16 * 1024)
#define MAX_SIZE (16 * 1024 * 1024)

//TODO: 创建数据结构
int left = 0;
int right = 0;
int pmm_lock = UNLOCKED;

static void *kalloc(size_t size) {
    lock(&pmm_lock);
    if (size == 0) {
        return NULL;
    } else if (size < 16) { // 最小单元16字节
        size = 16;
    } else if (size > MAX_SIZE) { 
        return NULL;
    } else { // 16 ~ 16KiB
        size_t align = 16;
        while (align < size) {
            align *= 2;
        }
        size = align;
        debug("size: %d\n", size); //TODO: 我的klib要实现一下%ld
    }
    int offset = 0;
    while (offset <= left) {
        offset += size;
    }
    left = offset;

    void *ret = NULL;
    //TODO: 实现指针偏移
    ret = heap.start + offset;
    unlock(&pmm_lock);

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
}
#else
#include <stdio.h>
#define HEAP_SIZE (125 * 1024 * 1024)
static void pmm_init() {
    char *ptr = malloc(HEAP_SIZE);
    heap.start = ptr;
    heap.end = ptr + HEAP_SIZE;
    printf("Got %d MiB heap: [%p, %p)\n", HEAP_SIZE >> 20, heap.start, heap.end);
}
#endif

MODULE_DEF(pmm) = {
    .init  = pmm_init,
    .alloc = kalloc,
    .free  = kfree,
};