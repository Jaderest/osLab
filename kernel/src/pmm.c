#include <common.h>

#define MAX_CPU 8

//TODO: 自旋锁
#define UNLOCKED 0
#define LOCKED 1
typedef struct lock_t {
    int flag;
} lock_t;
void lock_init(lock_t *lock) {
    atomic_xchg(&lock->flag, LOCKED);
}
void lock(lock_t *lock) {
    while(atomic_xchg(&lock->flag, LOCKED) == LOCKED) {/*spin*/};
}
void unlock(lock_t *lock) {
    panic_on(atomic_xchg(&lock->flag, UNLOCKED) != LOCKED, "unlock failed");
}

#define PAGE_SIZE (16 * 1024)


static void *kalloc(size_t size) {
    if (size == 0) {
        return NULL;
    } else if (size < 16) { // 最小单元16字节
        size = 16;
    } else if (size > PAGE_SIZE) { 

    } else { // 16 ~ 16KiB
        size_t align = 16; //?是否可以优化
        while (align < size) {
            align *= 2;
        }
        size = align;
        printf("size: %d\n", size);
    }

    void *ret = NULL;
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
#define HEAP_SIZE (128 * 1024 * 1024)
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