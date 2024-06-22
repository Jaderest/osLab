#include <common.h>

#define MAX_CPU 8

#ifdef TEST
#include <am.h>
void putch(char ch) {
    putchar(ch);
}
#endif


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

lock_t pmm_lock;
void *start;

static void *kalloc(size_t size) {
    int align = 1;
    while (align < size) {
        align *= 2;
    }
    if (align > 16 * 1024 * 1024) {
        return NULL;
    }
    
    lock(&pmm_lock);
    void *ret = start;
    printf("alloc %d bytes at %p\n", align, ret);
    start += align;
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
    start = heap.start;
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