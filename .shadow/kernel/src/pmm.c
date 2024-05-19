#include <common.h>

#define MAX_CPU 8

#define UNAVAILABLE 0
#define AVAILABLE 1
//TODO: 自旋锁
typedef struct lock_t {
    int flag;
} lock_t;
void lock_init(lock_t *lock) {
    lock->flag = AVAILABLE;
}
void lock(lock_t *lock) {
    while(atomic_xchg(&lock->flag, UNAVAILABLE) == UNAVAILABLE) {
        // spin
    };
    assert(lock->flag = UNAVAILABLE);
}

void unlock(lock_t *lock) {
    assert(lock->flag = UNAVAILABLE);
    atomic_xchg(&lock->flag, AVAILABLE);
}

#define PAGE_SIZE (16 * 1024)


static void *kalloc(size_t size) {
    if (size == 0) {
        return NULL;
    } else if (size < 16) { // 最小单元16字节
        size = 16;
    } else if (size > PAGE_SIZE) { 

    } else { // 16 ~ 16KiB

        printf("size: %d\n", size);
    }

    void *ret = NULL;
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