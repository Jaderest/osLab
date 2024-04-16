#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>


#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

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
  va_list args; // 声明一个指向可变参数列表的对象
  va_start(args, fmt); // 宏，用于初始化va_list变量args，

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
        default:
          putch(*fmt);
          break;
      }
    } else {
      putch(*fmt);
    }

    fmt++;
  }
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
