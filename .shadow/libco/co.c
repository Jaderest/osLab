#include "co.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#ifdef LOCAL_MACHINE
    #define debug(...) printf(__VA_ARGS__)
#else
    #define debug(...)
#endif

struct context {
    // 上下文切换，把寄存器都保存下来
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
    struct context ctx;
    uint8_t stack[4096]; // 协程的堆栈
};

struct co *co_start(const char *name, void (*func)(void *), void *arg) {
    return NULL;
}

void co_wait(struct co *co) {
    
}

void co_yield() {
    
}