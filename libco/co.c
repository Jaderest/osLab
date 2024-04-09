#include "co.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <setjmp.h>
#include <stdio.h>

#ifdef LOCAL_MACHINE
    #define debug(...) printf(__VA_ARGS__)
#else
    #define debug(...)
#endif

enum co_status {
    CO_NEW = 1, // 新创建的协程
    CO_RUNNING, // 正在运行的协程
    CO_WAITING, // 等待其他协程的协程
    CO_DEAD // 已经结束的协程
};

struct context {
    //TODO
    uint64_t rax;
    uint64_t rbx;
    uint64_t rcx;
    uint64_t rdx;
    uint64_t rsi;
    uint64_t rdi;
    uint64_t rbp;
    uint64_t rsp; // 栈顶指针
    uint64_t r8;
    uint64_t r9;
    uint64_t r10;
    uint64_t r11;
    uint64_t r12;
    uint64_t r13;
    uint64_t r14;
    uint64_t r15;
};

#define STACK_SIZE 1024 * 1024

struct co { // 需要分配内存，可以使用malloc，自己设计这个结构体
    char *name;
    void (*func)(void *);
    void *arg;
    // 每个协程想要执行，就需要拥有独立的堆栈和寄存器，一个协程的寄存器、堆栈、共享内存就构成了当前协程的状态机执行
    enum co_status status;
    struct co *waiter; // 是否有其他协程在等待当前协程
    struct context context; // 寄存器
    uint8_t *stack[STACK_SIZE]; // 堆栈
};

struct co *current = NULL; // 当前正在执行的协程

// 创建一个新的协程，返回一个指向struct co的指针
struct co *co_start(const char *name, void (*func)(void *), void *arg) {
    // 新状态机 %rsp 寄存器指向它独立的堆栈，%rip 寄存器指向 func 函数的地址
    //! 寄存器再看看呢

    struct co *co = malloc(sizeof(struct co));
    co->func = func;
    co->name = malloc(strlen(name) + 1);
    strcpy(co->name, name);
    co->arg = arg;
    co->status = CO_NEW;
    co->waiter = NULL;
    // TODO：初始化寄存器
    // TODO：初始化堆栈
    if (current == NULL) {
        current = co;
        // TODO：切换到新的协程
    } else {
        // TODO：将新的协程加入到队列中
    }
    return co;
}

void co_wait(struct co *co) { // 当前协程需要等待，知道co协程执行完成才能继续执行
    // 被等待的协程结束后，co_wait() 返回前，co_start 分配的struct co需要被释放
    // 因此每个 协程只能被 co_wait 一次
    // 等待状态机结束，即等待 func 函数执行完成
    co->waiter = current;
    current->status = CO_WAITING;
    // TODO：切换到另一个协程
    // TODO：恢复寄存器
    // TODO：释放 co 占用的内存
    // TODO：切换到当前协程
    // TODO：恢复寄存器
    // TODO：释放当前占用的内存
    co->status = CO_DEAD;
}

void co_yield() {
    // 实现协程的切换，当前协程放弃执行，若系统有多个可以运行的协程，选择一个继续执行（包括当前协程，但是不能回到当前函数）
    // main 函数的执行也是一个协程，因此在main中调用 co_yield() 或 co_wait() 时，程序结束
    // 不能把正在执行的函数返回，否则会导致栈被破坏
    // ? 那么我需要通过什么数据结构来存储当前的协程呢？
    // TODO：保存寄存器
    // TODO：切换到另一个协程
    // TODO：恢复寄存器

    // 把当前代码执行的状态机切换到另一段代码
    // 上下文切换，通过小心地用汇编代码保存ji和恢复寄存器的值，在最后执行pc的切换
// 可以用setjmp和longjmp来实现状态的恢复
    // int val = setjmp(*(jmp_buf*)&current->context); //TODO:仔细想想这里指针报错的原因呢
    // if (val == 0) {
    //     struct co *next = NULL; //TODO:找到下一个要跑的进程
    //     next->status = CO_RUNNING;

    //     // 将当前协程上下文保存到current->context
    //     longjmp(*(jmp_buf*)&next->context, 1);
    // } else {
    //     // 这里是从其他协程切换回来的地方
    // }
}
