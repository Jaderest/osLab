#include "co.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

enum co_status {
    CO_NEW = 1, // 新创建的协程
    CO_RUNNING, // 正在运行的协程
    CO_WAITING, // 等待其他协程的协程
    CO_DEAD // 已经结束的协程
};

struct context {
    //TODO
    // 寄存器
    // %rip 指令指针寄存器
    // %rsp 栈指针寄存器
    // %rbp 基址指针寄存器
    // %rax 寄存器
    // %rbx 寄存器
    // %rcx 寄存器
    // %rdx 寄存器
    // %rsi 寄存器
    // %rdi 寄存器
    // %r8 寄存器
    // %r9 寄存器
    // %r10 寄存器
    // %r11 寄存器
    // %r12 寄存器
    // %r13 寄存器
    // %r14 寄存器
    // %r15 寄存器
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

/*
    为每一个协程分配独立的堆栈，栈顶指针由 current->context 中的 %rsp 寄存器指定
    co_yield 时，将寄存器保存到属于该协程的 current->context 中，包括 %rsp
    切换到另一个协程执行，恢复另一个协程的寄存器，包括 %rsp
*/

// 创建一个新的协程，返回一个指向struct co的指针
struct co *co_start(const char *name, void (*func)(void *), void *arg) {
    // 新状态机 %rsp 寄存器指向它独立的堆栈，%rip 寄存器指向 func 函数的地址
    //! 寄存器再看看呢
    struct co *co = malloc(sizeof(struct co));
    co->func = func;
    strcpy(co->name, name);
    co->arg = arg;
    co->status = CO_NEW;
    co->waiter = NULL;
    // TODO：初始化寄存器
    // TODO：初始化堆栈
    return co;
}

void co_wait(struct co *co) { // 当前协程需要等待，知道co协程执行完成才能继续执行
    // 被等待的协程结束后，co_wait() 返回前，co_start 分配的struct co需要被释放
    // 因此每个 协程只能被 co_wait 一次
    // 等待状态机结束，即等待 func 函数执行完成
}

void co_yield() {
    // 实现协程的切换，当前协程放弃执行，若系统有多个可以运行的协程，选择一个继续执行（包括当前协程，但是不能回到当前函数）
    // main 函数的执行也是一个协程，因此在main中调用 co_yield() 或 co_wait() 时，程序结束
    // 不能把正在执行的函数返回，否则会导致栈被破坏
    // 上下文切换，通过小心地用汇编代码保存ji和恢复寄存器的值，在最后执行pc的切换
}
