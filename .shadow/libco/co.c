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
// #define debug(...)

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
    struct co* next;

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
        current->name = "main";
        current->waiter = NULL;
        current->func = NULL;
        current->arg = NULL;
        current->next = current;
    }

    // 换一种数据结构试一下
    struct co *node = current;
    while (node != NULL) {
        if (node->next == current) {
            break;
        }
        node = node->next;
    }
    
    // 其实同样是环状链表，刚刚那个双向链表有问题可能
    node->next = co;
    co->next = current;
    
    return co;
}

void co_wait(struct co *co) { // 当前协程需要等待 co 执行完成
    if (co == NULL) {
        return;
    }
    if (co->status == CO_DEAD) {
        free(co->name);
        free(co);
        return;
    }
    current->status = CO_WAITING;
    co->waiter = current;
    debug("co_wait: %s\n", co->name);

    while(co->status != CO_DEAD) { // 此时协程尚未结束，因此co_wait不能继续执行
        co_yield();
    }
    current->status = CO_RUNNING;

    struct co *node = current;
    while (node != NULL) {
        if (node->next == co) {
            break;
        }
        node = node->next;
    }
    node->next = co->next;

    free(co->name);
    free(co);
}


void co_yield() {
    if (current == NULL) {
        current = (struct co *)malloc(sizeof(struct co));
        current->status = CO_RUNNING;
        current->name = "main";
        current->func = NULL;
        current->arg = NULL;
        current->waiter = NULL;
    }
    assert(current != NULL);

    int val = setjmp(current->context);
    if (val == 0) { // 选择下一个待运行的协程、
        struct co *node = current->next;
        while (node != NULL && (node->status == CO_DEAD || node->status == CO_WAITING)) {
            node = node->next;
        }
        current = node;
        debug("co_yield: %s\n", current->name);
        if (current->status == CO_NEW) {
            ((struct co volatile *)current)->status = CO_RUNNING;
            stack_switch_call(current->stack + STACK_SIZE, current->func, (uintptr_t)current->arg);
            ((struct co volatile *)current)->status = CO_DEAD;
            if (current->waiter != NULL) {
                current = current->waiter;
            }
        } else {
            longjmp(current->context, 1);
        }
    } else {
        return;
    }
}

// 遍历当前的链表
// void traverse() {
//     co_node *node = head;
//     while (node != NULL) {
//         debug("traverse: %s\n", node->ptr->name);
//         node = node->next;
//         if (node == tail) {
//             break;
//         }
//     }
// }