#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>

#define ZEROPAD 1 // pad(padding) with zero
#define SIGN 2
#define PLUS 4 // show plus
#define SPACE 8
#define LEFT 16
#define SPECIAL 32

#define is_digit(c) ((c) >= '0' && (c) <= '9')

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

static int get_wid(const char **s) {
  int i = 0;
  while (is_digit(**s)) {
    i = i * 10 + *((*s)++) - '0';
  }
  return i;

}

static char* number2str(char *str, long num, int base, int size, int type) {
  char *digits = "0123456789abcdefghijklmnopqrstuvwxyz";
  char c, sign, tmp[70];
  int i = 0;
  c = (type & ZEROPAD) ? '0' : ' ';
  sign = 0;

  if (type & SIGN) {
    if (num < 0) {
      sign = '-';
      num = -num;
      size--;
    } else if (type & PLUS) {
      sign = '+';
      size--;
    } else if (type & SPACE) {
      sign = ' ';
      size--;
    }
  }

  if (num == 0) {
    tmp[i++] = '0';
  } else {
    while (num != 0) {
      tmp[i++] = digits[(unsigned long)num % (unsigned)base];
      num = ((unsigned long)num) / (unsigned)base;
    }
  }

  size -= i;

  if (!(type & (ZEROPAD | LEFT))) {
    while (size-- > 0) {
      *str++ = ' ';
    }
  }

  if (sign) {
    *str++ = sign;
  }

  if (!(type & LEFT)) {
    while (size-- > 0) { *str++ = c; }
  }
  while (i-- > 0) { *str++ = tmp[i]; }
  while (size-- > 0) { *str++ = ' '; }

  return str;
}


// klib-macro.h提供putstr
int printf(const char *fmt, ...) {
  char out[2048];
  va_list va;
  va_start(va, fmt);
  int ret = vsprintf(out, fmt, va);
  va_end(va);
  putstr(out);
  return ret;
}

int vsprintf(char *out, const char *fmt, va_list ap) {
  int len, i;
  unsigned long num;
  char *str;
  char *s;
  int flags = 0;  // 用来指示类型
  int integer_width; // 整数的长度 如%8d，8为精度
  int base;  // 进制

  for (str = out; *fmt; fmt++) {
    // 还没有出现需要转化的常规字符
    if (*fmt != '%') {
      *str++ = *fmt;
      continue;
      flags = 0;
    }
  repeat:
    fmt++;

    switch (*fmt) {
    case '-':
      flags |= LEFT;
      goto repeat;
    case '+':
      flags |= PLUS;
      goto repeat;
    case ' ':
      flags |= SPACE;
      goto repeat;
    case '0':
      flags |= ZEROPAD;
      goto repeat;
    }

    integer_width = -1;
    if (is_digit(*fmt)) {
      integer_width = get_wid(&fmt);
    }

    base = 10;
    switch (*fmt) {
      case 'p': // 指针
        if (integer_width == -1) {
          integer_width = 2 * sizeof(void *);
          flags |= ZEROPAD;
        }
        *str++ = '0', *str++ = 'x';
        str = number2str(str, (unsigned long)va_arg(ap, void *), 16, integer_width, flags);
        continue;
      case 'c': // 单字符
        *str++ = (unsigned char)va_arg(ap, int);
        continue;
      case 's': // 字符串
        s = va_arg(ap, char *);
        if (!s) {
          s = "(null)";
        }
        len = strlen(s);
        for (i = 0; i < len; i++) {
          *str++ = *s++;
        }
        continue;
      case 'o':
        base = 8;
        break;
      case 'x':
        base = 16;
        break;
      case 'd':
        flags |= SIGN;
      case 'u':
        break;
      
      default:  
        panic("Not implemented");
        if (*fmt == '%') {
          *str++ = '%';
        }
        if (*fmt) {
          *str++ = *fmt;
        } else {
          fmt--;
        }
        continue;
    }
    if (flags & SIGN) {
      num = va_arg(ap, int);
    } else {
      num = va_arg(ap, unsigned int);
    }

    str = number2str(str, num, base, integer_width, flags);
  }
  *str = '\0';
  return str - out;
}

int sprintf(char *out, const char *fmt, ...) {
  va_list va;
  va_start(va, fmt);
  int ret = vsprintf(out, fmt, va);
  va_end(va);
  return ret;
}

int snprintf(char *out, size_t n, const char *fmt, ...) {
  panic("Not implemented");
}

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap) {
  panic("Not implemented");
}

#endif
