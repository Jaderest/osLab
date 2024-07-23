#ifndef _COMMON_H__
#define _COMMON_H__

// #include <kernel.h>
// #include <klib.h>
// #include <klib-macros.h>
#include <assert.h>
#include <kernel.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define DEBUG
#ifdef DEBUG

// 使用可变参数的方式定义debug宏
#define debug(...) printf(__VA_ARGS__)

// TODO PANIC宏，记得改改
#define PANIC_ON(condition, s)                                                 \
    ({
if (condition) {
  putstr("Panic: ");
  putstr(s);
  putstr(" @ " __FILE__ ":" TOSTRING(__LINE__) "  \n");
  halt(1);
}
})

#define PANIC(s) PANIC_ON(1, s)

#else
#define debug(fmt, ...)
#define PANIC_ON(condition, s)
#define PANIC(s)
#endif

typedef struct {
  void *start, *end;
} Area;

#endif