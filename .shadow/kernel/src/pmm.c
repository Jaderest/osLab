#include <common.h>

#define MAX_CPU 8

#define AVAILABLE 1
#define UNAVAILABLE 0
typedef int lock_t;

void lock(lock_t *lock) {
    while (atomic_xchg(lock, UNAVAILABLE) != UNAVAILABLE) ;
    assert(*lock == UNAVAILABLE);
}
void unlock(lock_t *lock) {
    atomic_xchg(lock, AVAILABLE);
}
void init_lock(lock_t *lock) {
    *lock = AVAILABLE;
}

// 一个空闲块的结构
typedef struct freenode_t {
    struct freenode_t *next;
    int size;
    int magic;
} freenode_t;

// 每个CPU设计一个page
typedef struct pageheader_t { // 页头
    struct pageheader_t *next;
    int size;
    unsigned page_start; // 页中空闲链表起点
} pageheader_t;

typedef struct pagefreenode_t {
    struct pagefreenode_t *next;
    int size;
    int magic;
} pagefreenode_t;

typedef struct pagelist_t {
    pageheader_t *first; // CPU所属的页链表
    lock_t lock;
} pagelist_t;

lock_t heap_lock;
freenode_t *head;

#define PAGE_SIZE (64 * 1024)

// 我们通过空闲链表分配页（每个cpu），分配页：这个cpu第一次kalloc  or  当前所需内存块大小的page没有，所以分配一个new page
void kallocpage(pageheader_t **pagehead) { //一个page多大呢？
    lock(&heap_lock); // 对整个heap用一把大锁

    // 遍历heap，找到一个大于PAGE_SIZE的空闲块
    freenode_t *node = head;
    // freenode_t *prev = NULL;
    // 找到一个大于PAGE_SIZE的空闲块
    while (node != NULL) {
        if (node->size >= PAGE_SIZE) {
            break;
        }
        // prev = node;
        node = node->next;
    }

    if (node != NULL) { // 要在此分配一个page了
        // freenode_t *freenode = (void *)node + PAGE_SIZE;
        //TODO: 这里还没想好怎么分配
    }
}


static void *kalloc(size_t size) {
    if (size == 0) {
        return NULL;
    } else if (size < 16) { // 最小单元16字节
        size = 16;
    } else if (size > PAGE_SIZE) {//16KiB大内存
        if (size > 16 * 1024 * 1024) {
            return NULL;
        } // 16MiB
    } else { // 16 ~ 16KiB
        size_t align = 16;
        while (align < size) {
            align *= 2;
        }
        size = align;
        printf("size: %d\n", size);
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


/*
事实上，AbstractMachine 的程序运行在 freestanding 的环境下 (操作系统上的 C 程序运行在 hosted 环境下)：The __STDC_HOSTED__ macro expands to 1 on hosted implementations, or 0 on freestanding ones. The freestanding headers are: float.h, iso646.h, limits.h, stdalign.h, stdarg.h, stdbool.h, stddef.h, stdint.h, and stdnoreturn.h. You should be familiar with these headers as they contain useful declarations you shouldn't do yourself. GCC also comes with additional freestanding headers for CPUID, SSE and such.

这些头文件中包含了 freestanding 程序也可使用的声明。有兴趣的同学可以发现，可变参数经过预编译后生成了类似 __builtin_va_arg 的 builtin 调用，由编译器翻译成了特定的汇编代码。
*/