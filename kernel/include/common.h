#ifndef __COMMON_H__
#define __COMMON_H__


#include <kernel.h>
#include <klib.h>
#include <klib-macros.h>


#define MAX_CPU_NUM (8)

#define STACK_SIZE (8192)
#define STACK_GUARD_SIZE (4)
#define STACK_GUARD_VALUE (0xdeadbeef)

#define INT_MAX (0x7fffffff)
#define INT_MIN (0x80000000)


//------------------log------------------
#define LOG
#ifdef LOG
extern spinlock_t log_lk;
#define log(format, ...) \
    do { \
        _spin_lock(&log_lk); \
        printf(format, ##__VA_ARGS__); \
        _spin_unlock(&log_lk); \
    } while (0)
#else
#define log(format, ...)
#endif

//------------------assert------------------
#define ASSERT
#ifdef ASSERT
#define PANIC(fmt, ...)  \
    do {  \
        printf("\033[1;41m[cpu%d]Panic: %s:%d: " fmt "\033[0m\n", cpu_current(), __FILE__, __LINE__, ##__VA_ARGS__);  \
        while(1) asm volatile ("hlt");  \
    } while (0)

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

#define INTR PANIC_ON(!ienabled(), "Interrupt is disabled")
#define NO_INTR PANIC_ON(ienabled(), "Interrupt is enabled")

// #define TRACE_F
#ifdef TRACE_F_COLOR
    #define TRACE_ENTRY \
        log("\033[1;32m[TRACE in %d] %s: %s: %d: Entry\033[0m\n", cpu_current(), __FILE__, __func__, __LINE__)
    #define TRACE_EXIT \
        log("\033[1;32m[TRACE in %d] %s: %s: %d: Exit\033[0m\n", cpu_current(), __FILE__, __func__, __LINE__)
#elif defined(TRACE_F)
    #define TRACE_ENTRY \
        printf("[TRACE in %d] %s: %s: %d: Entry\n", cpu_current(), __FILE__, __func__, __LINE__)
    #define TRACE_EXIT \
        printf("[TRACE in %d] %s: %s: %d: Exit\n", cpu_current(), __FILE__, __func__, __LINE__)
#else
    #define TRACE_ENTRY
    #define TRACE_EXIT
#endif // TRACE_F



#endif // __COMMON_H__