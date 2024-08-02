#ifndef __COMMON_H__
#define __COMMON_H__


#include <kernel.h>
#include <klib.h>
#include <klib-macros.h>


#define MAX_CPU_NUM 8

// #define DEBUG
#ifdef DEBUG

#define debug(...) printf(__VA_ARGS__)
#else
#define debug(fmt, ...)
#endif

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
#endif

#endif