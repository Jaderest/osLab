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

#define STACK_SIZE 64 * 1024
#define MAX_CO 128

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

// 从头到尾，同时只有一个函数在被使用
struct co *co_start(const char *name, void (*func)(void *), void *arg) {
    // 创建一个新的状态机，仅此而已（堆栈和状态机保存在共享内存？）
    struct co *co = malloc(sizeof(struct co));
    co->name = malloc(strlen(name) + 1);
    strcpy(co->name, name);
    co->func = func;
    co->arg = arg;
    co->status = CO_NEW;
    co->waiter = NULL;
    memset(&co->context, 0, sizeof(co->context));
    memset(co->stack, 0, sizeof(co->stack));
    return co;
}

void co_wait(struct co *co) { // 当前协程需要等待 co 执行完成
    


    

    free(co->name);
    free(co);
}


void co_yield() {
    
}