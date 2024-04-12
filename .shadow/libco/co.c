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
#define MAX_CO 150

enum co_status {
    CO_NEW = 1, // 新创建，还未执行过
    CO_RUNNING, // 已经执行过
    CO_WAITING, // 在 co_wait 上等待
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
    struct co* waiter; // 是否有其他协程在等待当前协程，所以co->waiter = current
    jmp_buf context; // 寄存器现场
    uint8_t stack[STACK_SIZE]; // 协程的堆栈
};

// 全局指针，指向当前运行的协程
struct co* current = NULL;

// 需要用一个数据结构存储所有的协程
typedef struct co_node {
    struct co *ptr;
    struct co_node *next;
} co_node; 

co_node *head = NULL;

void append(struct co *co) {
    co_node *node = malloc(sizeof(co_node));
    node->ptr = co;
    node->next = NULL;
    if (head == NULL) {
        head = node;
    } else {
        co_node *p = head;
        while(p->next != NULL) {
            p = p->next;
        }
        p->next = node;
    }
}

void delete(struct co *co) {
    co_node *p = head;
    co_node *q = NULL;
    while(p != NULL) {
        if (p->ptr == co) {
            if (q == NULL) {
                head = p->next;
            } else {
                q->next = p->next;
            }
            free(p);
            break;
        }
        q = p;
        p = p->next;
    }
}

// 从头到尾，同时只有一个函数在被使用
struct co *co_start(const char *name, void (*func)(void *), void *arg) {
    // 创建一个新的状态机，仅此而已（堆栈和状态机保存在共享内存？）
    struct co *co = malloc(sizeof(struct co));
    assert(co != NULL);

    co->name = malloc(strlen(name) + 1);
    assert(co->name != NULL);

    strcpy(co->name, name);
    co->func = func;
    co->arg = arg;
    co->status = CO_NEW;
    co->waiter = NULL;
    memset(&co->context, 0, sizeof(co->context));
    memset(co->stack, 0, sizeof(co->stack));
    debug("co_start: %s\n", co->name);

    if (current == NULL) { // 第一个协程，其实这个就该是main函数
        current = (struct co *)malloc(sizeof(struct co));
        current->status = CO_RUNNING;
        current->waiter = NULL;
        current->name = "main";
        current->func = NULL;
        current->arg = NULL;
    }
    
    return co;
}

void co_wait(struct co *co) { // 当前协程需要等待 co 执行完成
    current->status = CO_WAITING;
    co->waiter = current;
    while(co->status != CO_DEAD) {
        co_yield();
    }
    current->status = CO_RUNNING;
}


void co_yield() {

}