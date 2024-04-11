#include "co.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <setjmp.h>
#include "am.h"

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

struct co* current = NULL;

struct co *co_start(const char *name, void (*func)(void *), void *arg) {
    return NULL;
}

void co_wait(struct co *co) {
    
}

void co_yield() { // 是一个函数调用，编译器会自动
    int val = setjmp(current->context.ctx);
    if (val == 0) {
        stack_switch_call(current->context.ctx, current->context.ctx, current->func, current->arg);
    } else {

    }
}