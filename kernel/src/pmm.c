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
    // printf("%x\n", (uintptr_t)heap.start);
    // printf("%x\n", (uintptr_t)heap.end);
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