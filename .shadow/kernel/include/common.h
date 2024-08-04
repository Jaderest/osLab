#ifndef __COMMON_H__
#define __COMMON_H__


#include <kernel.h>
#include <klib.h>
#include <klib-macros.h>


#define MAX_CPU_NUM (8)
#define MAX_THREAD (256)
#define STACK_SIZE (8192)
#define STACK_GUARD_SIZE (4)
#define STACK_GUARD_VALUE (0xdeadbeef)
#define INT_MAX (0x7fffffff)
#define INT_MIN (0x80000000)

#define INTR assert(ienabled())
#define NO_INTR assert(!ienabled())

#define ASSERT
#ifdef ASSERT

// PANIC宏
#define PANIC(fmt, ...)  \
    do {  \
        printf("\033[1;41mPanic: %s:%d: " fmt "\033[0m\n", __FILE__, __LINE__, ##__VA_ARGS__);  \
        while(1) asm volatile ("hlt");  \
    } while (0)

// PANIC_ON宏
#define PANIC_ON(condition, message, ...)         \
    do {                                          \
        if (condition) {                          \
            PANIC(message, ##__VA_ARGS__);        \
        }                                         \
    } while (0)

#else
#define PANIC(fmt, ...)
#define PANIC_ON(condition, message, ...)
#endif // ASSERT



#endif // __COMMON_H__