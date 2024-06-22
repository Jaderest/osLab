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
    CO_WAITING, // 等待其他协程
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
    struct co *waiter;
    jmp_buf context; // 寄存器现场
    uint8_t stack[STACK_SIZE]; // 协程的堆栈
};

// 全局指针，指向当前运行的协程
struct co* current = NULL;

struct co* costack[MAX_CO]; // 存放所有的协程
int co_num = 0; // 当前协程的数量

struct co* choose() {
    int waiter = 0;
    int a = rand() % 2;
    if (a == 0) {
        for (int i = 0; i < co_num; i++) {
            if (costack[i]->status == CO_WAITING) {
                waiter = i;
                continue;
            }
            if (costack[i]->status != CO_DEAD && current != costack[i]) {
                return costack[i];
            }
        }
    } else {
        for (int i = co_num - 1; i >= 0; i--) {
            if (costack[i]->status == CO_WAITING) {
                waiter = i;
                continue;
            }
            if (costack[i]->status != CO_DEAD && current != costack[i]) {
                return costack[i];
            }
        }
    }
    return costack[waiter];
}
void append(struct co* co) {
    assert(co_num < MAX_CO);
    costack[co_num++] = co;
}
void delete(struct co* co) {
    if (co_num < 1) {
        return;
    }
    for (int i = 0; i < co_num; i++) {
        if (costack[i] == co) { // TODO？
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

struct co *co_start(const char *name, void (*func)(void *), void *arg) {
    struct co *co = (struct co *)malloc(sizeof(struct co));
    assert(co != NULL);
    co->name = strdup(name);
    co->func = func;
    co->arg = arg;
    co->status = CO_NEW; // 新创建的协程
    co->waiter = NULL;
    append(co);
    return co;
}

void co_wait(struct co *co) { // 当前协程需要等待 co 执行完成
    while (1) {
        if (co->status == CO_DEAD) {
            delete(co);
            free(co->name);
            free(co);
            int i = 0;
            for (i = 0; i < co_num; i++) {
                if (costack[i] == current) { // 即如果有协程在等待当前协程
                    break;
                }
            }
            if (i == co_num) {
                current->status = CO_RUNNING; // 恢复当前协程的状态
            }
            break; // 那么回到当前current协程
        } else {
            co->waiter = current;
            current->status = CO_WAITING;
            co_yield();
        }
    }
}

void co_wrapper(struct co *co) {
    co->func(co->arg);
}

// static void co_finish() {
//     current->status = CO_DEAD;
//     if (current->waiter != NULL) {
//         current = current->waiter;
//         longjmp(current->context, 0);
//     } else {
//         struct co *next = choose();
//         current = next;
//         if (current->status == CO_NEW) {
//             next->status = CO_RUNNING;
//             stack_switch_call(&current->stack[STACK_SIZE], co_wrapper, (uintptr_t)current);
//         } else {
//             longjmp(current->context, 1);
//         }
//     }
// }

void co_yield() {
    int val = setjmp(current->context);
    if (val == 0) {
        struct co *next = choose();
        current = next;
        if (current->status == CO_NEW) {
            next->status = CO_RUNNING;
            debug("co_yield: %s\n", current->name);
            stack_switch_call(&current->stack[STACK_SIZE], co_wrapper, (uintptr_t)current);
        } else {
            longjmp(current->context, 1);
        }
    } else {
        return;
    }
}