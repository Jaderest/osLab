#include "co.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <setjmp.h>
#include <assert.h>
#include <time.h>

#define DEBUG
#ifdef DEBUG
    #define debug(...) printf(__VA_ARGS__)
#else
    #define debug(...)
#endif

#define STACK_SIZE 64 * 1024
#define MAX_CO 150

enum co_status {
    CO_NEW = 1, // 新创建，还未执行过
    CO_RUNNING, // 已经执行过
    CO_DEAD,    // 已经结束，但还未释放资源
};

static inline void stack_switch_call(void *sp, void *entry, uintptr_t arg)
{
	asm volatile(
#if __x86_64__
		"movq %%rsp,-0x10(%0); leaq -0x20(%0), %%rsp; movq %2, %%rdi ; call *%1; movq -0x10(%0) ,%%rsp;"
		:
		: "b"((uintptr_t)sp), "d"(entry), "a"(arg)
		: "memory"
#else
		"movl %%esp, -0x8(%0); leal -0xC(%0), %%esp; movl %2, -0xC(%0); call *%1;movl -0x8(%0), %%esp"
		:
		: "b"((uintptr_t)sp), "d"(entry), "a"(arg)
		: "memory"
#endif
	);
}

struct co {
    char *name;
    void (*func)(void *); // co_start 指定的入口地址和函数
    void *arg;

    enum co_status status;
    jmp_buf context; // 寄存器现场
    uint8_t stack[STACK_SIZE]; // 协程的堆栈
};

// 全局指针，指向当前运行的协程
struct co* current = NULL;

struct co* costack[MAX_CO]; // 存放所有的协程
int co_num = 0; // 当前协程的数量

void append(struct co* co) {
    assert(co_num < MAX_CO);
    costack[co_num++] = co;
}
void delete(struct co* co) {
    for (int i = 0; i < co_num; i++) {
        if (costack[i] == co) {
            for (int j = i; j < co_num - 1; j++) {
                costack[j] = costack[j + 1];
            }
            co_num--;
            return;
        }
    }
}

void __attribute__((constructor)) co_init() {
    debug("co_init\n");
    current = (struct co *)malloc(sizeof(struct co));
    assert(current != NULL);
    current->name = "main";
    current->status = CO_RUNNING;
    current->func = NULL;
    current->arg = NULL;
    append(current);
}

void co_wrapper(struct co* co) {
    co->func(co->arg);
    co->status = CO_DEAD;
    delete(co);
}

struct co *co_start(const char *name, void (*func)(void *), void *arg) {
    struct co *co = (struct co *)malloc(sizeof(struct co));
    assert(co != NULL);
    co->name = strdup(name);
    co->func = func;
    co->arg = arg;
    co->status = CO_NEW; // 新创建的协程
    append(co);
    return co;
}

void co_wait(struct co *co) { // 当前协程需要等待 co 执行完成
    while (co->status != CO_DEAD) {
        debug("co_wait\n");
        co_yield();
    }
    free(co);
}

void co_yield() {
    int val = setjmp(current->context);
    if (val == 0) {
        int next = rand() % co_num;
        struct co *next_co = costack[next];
        if (next_co->status == CO_DEAD) {
            return;
        }
        current = next_co;
        current->status = CO_RUNNING;
        stack_switch_call(current->stack + STACK_SIZE, co_wrapper, (uintptr_t)current);
    } else {
        return;
    }
}

void traverse() {
    for (int i = 0; i < co_num; i++) {
        debug("co[%d]: %s\n", i, costack[i]->name);
    }
}