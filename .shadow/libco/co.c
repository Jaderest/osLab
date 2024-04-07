#include "co.h"
#include <stdlib.h>
#include <string.h>

typedef struct co { // 需要分配内存，可以使用malloc
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

void co_wait(struct co *co) {
}

void co_yield() {
}
