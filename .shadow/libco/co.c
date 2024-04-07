#include "co.h"
#include <stdlib.h>
#include <string.h>

typedef struct co { // 需要分配内存，可以使用malloc，自己设计这个结构体
    void (*func)(void *);
    void *arg;
    char name[16];
} Co;

// 创建一个新的协程，返回一个指向struct co的指针
struct co *co_start(const char *name, void (*func)(void *), void *arg) {
    Co *co = malloc(sizeof(Co));
    co->func = func;
    strcpy(co->name, name);
    co->arg = arg;
    return NULL;
}

void co_wait(struct co *co) { // 当前协程需要等待，知道co协程执行完成才能继续执行
    // 被等待的协程结束后，co_wait() 返回前，co_start 分配的struct co需要被释放
    // 因此每个 协程只能被 co_wait 一次
}

void co_yield() {
    // 实现协程的切换，当前协程放弃执行，若系统有多个可以运行的协程，选择一个继续执行（包括当前协程）
    // main 函数的执行也是一个协程，因此在main中调用 co_yield() 或 co_wait() 时，程序结束
}
