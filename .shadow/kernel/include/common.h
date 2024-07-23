#ifndef _COMMON_H__
#define _COMMON_H__

#include <kernel.h>
#include <klib-macros.h>
#include <klib.h>

#define DEBUG
#ifdef DEBUG

// 使用可变参数的方式定义debug宏
#define debug(...) printf(__VA_ARGS__)

// PANIC宏

// PANIC_ON宏
#define PANIC_ON(condition, s)  \
    ({  \
if (condition) { \
  putstr("Panic: ");  \
  putstr(s);  \
  putstr(" @ " __FILE__ ":" TOSTRING(__LINE__) "  \n");  \
  halt(1);  \
} })

#define PANIC(s) PANIC_ON(1, s)

#else
#define debug(fmt, ...)
#define PANIC(fmt, ...)
#define PANIC_ON(condition, message, ...)
#endif

#endif