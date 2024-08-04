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
/**
 * @brief 用于标记一个cpu的状态
 * 当中断到来时（也即进入on_interrupt时），都检查中断是否为关闭状态（应为关闭状态）
 * 持有锁的时候没中断，出来的时候没有中断
 * 主循环的时候中断都是打开状态，持有任意一把自旋锁的时候中断都处于关闭状态
 */
#define INTR assert(ienabled())
#define NO_INTR assert(!ienabled())

// #define DEBUG
#ifdef DEBUG

#define debug(...) printf(__VA_ARGS__)
#else
#define debug(fmt, ...)
#endif // DEBUG

// #define ASSERT
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