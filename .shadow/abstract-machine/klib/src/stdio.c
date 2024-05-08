#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>


#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

#define AVAILABLE 1
#define UNAVAILABLE 0
typedef int iolock_t; // 命名约定：_t表示自定义的数据类型

void iolock(iolock_t *lock) {
    while (atomic_xchg(lock, UNAVAILABLE) != UNAVAILABLE) ;
    assert(*lock == UNAVAILABLE);
}
void iounlock(iolock_t *lock) {
    atomic_xchg(lock, AVAILABLE);
}
void init_iolock(iolock_t *lock) {
    *lock = AVAILABLE;
}

iolock_t io_lock;

void putint(int n) {
  if (n < 0) {
    putch('-');
    n = -n;
  }
  if (n / 10) {
    putint(n / 10);
  }
  putch(n % 10 + '0');
}

// klib-macro.h提供putstr
int printf(const char *fmt, ...) {
  va_list args; // 声明一个指向可变参数列表的对象，即参数中的...
  va_start(args, fmt); // 宏，用于初始化va_list变量args，源是fmt

  iolock(&io_lock);
  while (*fmt) {
    if (*fmt == '%') {
      fmt++;
      switch (*fmt) {
        case 'd': {
          putint(va_arg(args, int));
          break;
        }
        case 's': {
          putstr(va_arg(args, const char *));
          break;
        }
        case 'c': {
          putch(va_arg(args, int));
          break;
        }
        case 'x': {
          putch('0');
          putch('x');
          int num = va_arg(args, int);
          for (int i = 24; i >= 0; i -= 4) {
            putch("0123456789abcdef"[(num >> i) & 0xf]); // 0xf = 0b1111，也即取出num的每4位
          }
          break;
        }
        case 'p': {
          putch('0');
          putch('x');
          uintptr_t num = va_arg(args, uintptr_t);
          for (int i = 24; i >= 0; i -= 4) {
            putch("0123456789abcdef"[(num >> i) & 0xf]);
          }
          break;
        }
        default:
          putch(*fmt);
          break;
      }
    } else {
      putch(*fmt);
    }

    fmt++;
  }
  iounlock(&io_lock);
  va_end(args); // 宏，用于清理va_list变量args
  return 0;
}

int vsprintf(char *out, const char *fmt, va_list ap) {
  panic("Not implemented");
}

int sprintf(char *out, const char *fmt, ...) {
  panic("Not implemented");
}

int snprintf(char *out, size_t n, const char *fmt, ...) {
  panic("Not implemented");
}

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap) {
  panic("Not implemented");
}

#endif
