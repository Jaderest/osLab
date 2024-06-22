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

#define STACK_SIZE (1 << 16)
#define MAX_CO 150

enum co_status {
    CO_NEW,
    CO_RUNNING,
    CO_DEAD
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
    void (*func)(void *); // coroutine function
    void *arg; // coroutine function argument

    enum co_status status;
    jmp_buf ctx;
    uint8_t stack[STACK_SIZE];
} ;

struct co *current = NULL;

struct co_stack {
    struct co *q[MAX_CO];
    int size;
} co_stack;

void append(struct co *co) {
    co_stack.q[co_stack.size] = co;
    co_stack.size++;
}

void delete(struct co *co) {
    for (int i = 0; i < co_stack.size; i++) {
        if (co_stack.q[i] == co) {
            for (int j = i; j < co_stack.size - 1; j++) {
                co_stack.q[j] = co_stack.q[j + 1];
            }
            co_stack.size--;
            return;
        }
    }
}

void __attribute__((constructor)) co_init() {
    current = co_start("main", NULL, NULL);
    current->status = CO_RUNNING;
}

void co_wrapper(struct co *co) {
    co->func(co->arg);
    co->status = CO_DEAD; // 运行完就可以删了
    delete(co);
    co_yield();
}

struct co* co_start(const char *name, void (*func)(void *), void *arg) {
    struct co *co = (struct co *)malloc(sizeof(struct co));
    if (co == NULL) {
        return NULL;
    }
    co->name = strdup(name);
    co->func = func;
    co->arg = arg;
    co->status = CO_NEW;
    memset(co->stack, 0, STACK_SIZE);
    append(co);
    return co;
}

void co_wait(struct co *co) {

}

void co_yield() {

}