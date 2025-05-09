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

// #define DEBUG
#ifdef DEBUG
  #define debug(...) printf(__VA_ARGS__)
#else
  #define debug(fmt, ...)
#endif

#define ASSERT
#ifdef ASSERT

// TODO PANIC宏，记得改改
#define PANIC(fmt, ...)                                                        \
  do {                                                                         \
    fprintf(stderr, "\033[1;41mPanic: %s:%d: " fmt "\033[0m\n", __FILE__,      \
            __LINE__, ##__VA_ARGS__);                                          \
    _exit(1);                                                                   \
  } while (0)

// PANIC_ON宏
#define PANIC_ON(condition, message, ...)                                      \
  do {                                                                         \
    if (condition) {                                                           \
      PANIC(message, ##__VA_ARGS__);                                           \
    }                                                                          \
  } while (0)

#else
#define PANIC(fmt, ...)
#define PANIC_ON(condition, message, ...)
#endif

typedef struct {
  void *start, *end;
} Area;

#endif