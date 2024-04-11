#include "co.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <setjmp.h>

#ifdef LOCAL_MACHINE
    #define debug(...) printf(__VA_ARGS__)
#else
    #define debug(...)
#endif

struct context {
    // 上下文切换，把寄存器都保存下来
    jmp_buf ctx;
};

enum co_status {
    CO_NEW = 1,
    CO_RUNNING,
    CO_WAITING,
    CO_DEAD,
};

struct co {
    char *name;
    void (*func)(void *); // co_start 指定的入口地址和函数
    void *arg;

    enum co_status status;
    struct co* waiter;
    struct context context;
    uint8_t stack[4096]; // 协程的堆栈
};

static inline void 
stack_switch_call(void *sp, void *entry, uintptr_t arg) { // 堆栈指针，入口地址，参数
    __asm__ volatile (
#if __x86_64__
        "movq %0, %%rsp; movq %2, %%rdi; jmp *%1"
          :
          : "b"((uintptr_t)sp),
            "d"(entry),
            "a"(arg)
          : "memory"
#else
        "movl %0, %%esp; movl %2, 4(%0); jmp *%1"
          :
          : "b"((uintptr_t)sp - 8),
            "d"(entry),
            "a"(arg)
          : "memory"
#endif
    );
}

struct co* current = NULL;

struct co *co_start(const char *name, void (*func)(void *), void *arg) {
    struct co *co = malloc(sizeof(struct co));
    co->name = strdup(name);
    debug("co_start: %s\n", co->name);
    co->func = func;
    co->arg = arg;
    co->status = CO_NEW;
    co->waiter = NULL;
    memset(&co->context, 0, sizeof(co->context));
    memset(co->stack, 0, sizeof(co->stack));
    co->stack[sizeof(co->stack) - 1] = 0; // 栈底设置为0

    stack_switch_call(co->stack + sizeof(co->stack) - 1, co->func, (uintptr_t)co->arg);
    return co;
}

void co_wait(struct co *co) { // 当前协程需要等待 co 执行完成
    if (co->status == CO_DEAD) {
        free(co);
    } else {
        current->status = CO_WAITING;
        current->waiter = co;
        co_yield();
    }
}

struct co* select_next_coroutine() { // 选择下一个协程
    // struct co *co = current;
    // if (current->status == CO_DEAD) {
    //     free(current);
    //     current = NULL;
    // } else {
    //     current = current->waiter;
    // }
    return NULL;
}


void co_yield() {
    // int val = setjmp(current->context.ctx);
    // if (val == 0) {
    //     struct co *next = select_next_coroutine();
    //     if (next != NULL) {
    //         debug("co_yield: %s\n", next->name);
    //         next->status = CO_RUNNING;
    //         longjmp(next->context.ctx, 1);
    //         stack_switch_call(next->stack + sizeof(next->stack) - 1, next->func, (uintptr_t)next->arg);
    //     }
    // } else {
    //     debug("co_yield: %s\n", current->name);
    //     return;
    // }
}