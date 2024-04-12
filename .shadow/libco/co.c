#include "co.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <setjmp.h>
#include <assert.h>

#ifdef LOCAL_MACHINE
    #define debug(...) printf(__VA_ARGS__)
#else
    #define debug(...)
#endif

#define STACK_SIZE 64 * 1024
#define MAX_CO 128

#define panic(cond, words) printf("Panic: %s\t", words); \
assert(cond);

enum co_status {
    CO_NEW = 1, // 新创建，还未执行过
    CO_RUNNING, // 已经执行过
    CO_WAITING, // 在 co_wait 上等待
    CO_DEAD,    // 已经结束，但还未释放资源
};

struct co {
    char *name;
    void (*func)(void *); // co_start 指定的入口地址和函数
    void *arg;

    enum co_status status;
    struct co* waiter; // 是否有其他协程在等待当前协程，所以co->waiter = current
    jmp_buf context; // 寄存器现场
    uint8_t stack[STACK_SIZE]; // 协程的堆栈
};

// 全局指针，指向当前运行的协程
struct co* current = NULL;

// 从头到尾，同时只有一个函数在被使用
struct co *co_start(const char *name, void (*func)(void *), void *arg) {
    // 创建一个新的状态机，仅此而已（堆栈和状态机保存在共享内存？）
    struct co *co = malloc(sizeof(struct co));
    panic(co != NULL, "co malloc failed");

    co->name = malloc(strlen(name) + 1);
    panic(co->name != NULL, "co->name malloc failed");

    strcpy(co->name, name);
    co->func = func;
    co->arg = arg;
    co->status = CO_NEW;
    co->waiter = NULL;
    memset(&co->context, 0, sizeof(co->context));
    memset(co->stack, 0, sizeof(co->stack));
    debug("co_start: %s\n", co->name);
    return co;
}

void co_wait(struct co *co) { // 当前协程需要等待 co 执行完成
    if (co->status == CO_DEAD) {
        free(co->name);
        free(co);
        return;
    }
    //? 怎么写啊wdnmd

}


void co_yield() {
    int val = setjmp(current->context); // 保存好了寄存器现场，将栈帧保存在jmp_buf中，然后通过longjmp在指定位置恢复出来，有点类似于goto
    if (val == 0) {
        // 此时需要选择下一个待运行的协程，相当于修改current，并切换到它
    } else {
        // setjmp 由另一个 longjmp 返回的，此时一定是某个协程调用 co_yield()，此时代表了寄存器
        return;
    }
}