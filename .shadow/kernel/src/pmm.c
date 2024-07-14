#include <common.h>

#define MAX_CPU 8

#define DEBUG
#ifdef DEBUG
    #define debug(...) printf(__VA_ARGS__)
#else
    #define debug(...)
#endif

#define LOCKED 1
#define UNLOCKED 0


void lock(int *lock) {
    while(atomic_xchg(lock, LOCKED) == LOCKED) {/*spin*/};
}
void unlock(int *lock) {
    panic_on(atomic_xchg(lock, UNLOCKED) != LOCKED, "unlock failed");
}

#define PAGE_SIZE (4 * 1024)
#define MAX_SIZE (16 * 1024 * 1024)

//TODO: 创建数据结构
int left = 0;
int pmm_lock = UNLOCKED;

static void *kalloc(size_t size) {
    lock(&pmm_lock);
    if (size == 0) {
        return NULL;
    } else if (size < 16) { // 最小单元16字节
        size = 16;
    } else if (size > MAX_SIZE) { 
        return NULL;
    } else if (size > PAGE_SIZE) {
        size_t align = PAGE_SIZE;
        while (align < size) {
            align *= 2;
        }
        size = align;
        debug("size: %d\n", size);
    } else { // 16 ~ 16KiB
        size_t align = 16;
        while (align < size) {
            align *= 2;
        }
        size = align;
        debug("size: %d\n", size); //TODO: 我的klib要实现一下%ld
    }
    void *ret = NULL;
    int offset = 0;
    while (offset < left) {
        offset += size; //保证指针对齐
    }
    if (offset + size > MAX_SIZE) {
        ret = NULL;
    } else {
        ret = heap.start + offset;
        left = offset + size;
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
}

MODULE_DEF(pmm) = {
    .init  = pmm_init,
    .alloc = kalloc,
    .free  = kfree,
};