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

co_node *head = NULL; // 单向循环链表
co_node *tail = NULL;

void append(struct co *co) {
    co_node *node = (co_node *)malloc(sizeof(co_node));
    assert(node != NULL);
    // debug("append: %s\n", co->name);
    node->ptr = co;
    node->next = NULL;

    if (head == NULL) {
        head = node;
        tail = node;
        node->next = head;
    } else {
        tail->next = node;
        node->next = head;
        tail = node;
    }
}

void delete(struct co *co) { // 仅从链表删除，空间释放不在这里
    // co_node *node = head;
    // while (node != NULL) {
    //     if (node->ptr == co) {
    //         if (node == head) {
    //             head = head->next;
    //             tail->next = head;
    //         } else {
    //             co_node *prev = head;
    //             while (prev->next != node) {
    //                 prev = prev->next;
    //             }
    //             prev->next = node->next;
    //         }
    //         free(node);
    //         break;
    //     }
    //     node = node->next;
    // }
    co_node *node = head;
    co_node *prev = NULL;
    while (node != NULL) {
        if (node->ptr == co) {
            if (node == head) {
                head = head->next;
                if (head == NULL) tail = NULL;
                else tail->next = head;
            } else {
                prev->next = node->next;
                if (node == tail) tail = prev;
            }
            free(node);
            break;
        }
        prev = node;
        node = node->next;
    }
    
}

// 从头到尾，同时只有一个函数在被使用
struct co *co_start(const char *name, void (*func)(void *), void *arg) {
    // 创建一个新的状态机，仅此而已（堆栈和状态机保存在共享内存？）
    struct co *co = malloc(sizeof(struct co));
    assert(co != NULL);

    co->name = strdup(name);
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
        append(current);
    }

    append(co);
    return co;
}

void co_wait(struct co *co) { // 当前协程需要等待 co 执行完成
    assert(co != NULL);
    if (co->status == CO_DEAD) {
        delete(co);
        free(co->name);
        free(co);
        return;
    }
    current->status = CO_WAITING;
    co->waiter = current;

    while(co->status != CO_DEAD) { // 不断切换可执行的线程执行，直到 co 执行完成
        debug("co_wait1: %s\n", co->name);
        co_yield();
        // debug("1\n");
    }
    current->status = CO_RUNNING;

    debug("co_wait: %s finished\n", co->name);
    delete(co);
    free(co->name);
    free(co);
}

co_node *choose_next() {
    co_node *node_next = head->next; // head 是 main

    int random = rand() % 127;
    for (int i = 0; i < random; i++) { // 随机化初始点
        node_next = node_next->next;
    }
    // 事实上第一个它都没进这个循环
    while (node_next->ptr->status == CO_DEAD || node_next->ptr->status == CO_WAITING) {
        node_next = node_next->next;
    }

    return node_next;
}

void co_yield() {
    if (current == NULL) {
        current = (struct co *)malloc(sizeof(struct co));
        current->status = CO_RUNNING;
        current->name = "main";
        current->func = NULL;
        current->arg = NULL;
        current->waiter = NULL;
        append(current);
    }
    assert(current != NULL);

    int val = setjmp(current->context);
    // debug("val: %d\n", val);
    // traverse();
    if (val == 0) { // 选择下一个待运行的协程
        co_node *node_next = choose_next();
        // debug("choose finished: %s\n", node_next->ptr->name);
        // 但是为什么会卡在这里
        assert(val == 0);

        current = node_next->ptr;

        if (node_next->ptr->status == CO_NEW) {
            ((struct co volatile*)current)->status = CO_RUNNING; // 真的是优化的问题...

            stack_switch_call(&current->stack[STACK_SIZE], node_next->ptr->func, (uintptr_t)node_next->ptr->arg);
            //! 最重要的一步，你代码甚至没有结束
            ((struct co volatile*)current)->status = CO_DEAD;
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

// 遍历当前的链表s，这下链表终于好了
void traverse() {
    debug("\n------traverse------\n");
    co_node *node = head;
    do {
        debug("%s -> ", node->ptr->name);
        node = node->next;
    } while (node != tail);
    debug("%s\n", node->ptr->name);
    debug("--------------------\n");
}

void detect() {
    debug("------detect------\n");
    debug("current: %s\n", current->name);
    debug("head: %s\n", head->ptr->name);
    debug("tail: %s\n", tail->ptr->name);
    debug("------detect------\n");
}

void detect2() {
    debug("------detect2------\n");
    debug("is head->next == thd1: %d\n", head->next == tail);
    debug("------detect2------\n");
}

void detect3() {
    debug("------detect3------\n");
    debug("is head->next == thd2: %d\n", head->next->next == tail);
    debug("------detect3------\n");
}