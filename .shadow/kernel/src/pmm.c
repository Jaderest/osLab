#include <common.h>

static void *kalloc(size_t size) {
    // TODO
    // You can add more .c files to the repo.
    if (size >= 16 * 1024 * 1024) {
        printf("kalloc: 16MiB too large\n");
        return NULL;
    }

    return NULL;
}

static void kfree(void *ptr) {
    // TODO
    // You can add more .c files to the repo.
}

static void pmm_init() {
    uintptr_t pmsize = (
        (uintptr_t)heap.end
        - (uintptr_t)heap.start
    );

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
