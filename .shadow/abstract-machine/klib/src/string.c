#include <klib.h>
#include <klib-macros.h>
#include <stdint.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

size_t strlen(const char *s) {
  size_t len = 0;
  while(s[len] != '\0') {
    len++;
  }
  return len;
}

char *strcpy(char *dst, const char *src) { // 单线程ok
  assert(dst != NULL && src != NULL);
  char *temp = dst;
  while ((*dst++ = *src++) != '\0');
  return temp;
}

char *strncpy(char *dst, const char *src, size_t n) { // 如果src长度小于n，则运行会发生Segmentation fault
  assert(dst != NULL && src != NULL);
  char *temp = dst;
  while (n-- && (*dst++ = *src++) != '\0');
  return temp;
}

char *strcat(char *dst, const char *src) { //string concatenate
  panic("Not implemented");
}

int strcmp(const char *s1, const char *s2) {
  while(*s1 && *s2 && *s1 == *s2) {
    s1++;
    s2++;
  }
  return *s1 - *s2;
}

int strncmp(const char *s1, const char *s2, size_t n) {
  while(*s1 && *s2 && *s1 == *s2 && n--) {
    s1++;
    s2++;
  }
  return *s1 - *s2;
}

void *memset(void *s, int c, size_t n) {
  const unsigned char uc = c;
  unsigned char *su;
  for (su = s; 0 < n; ++su, --n) {
    *su = uc;
  }
  return s;
}

void *memmove(void *dst, const void *src, size_t n) {
  char *d = dst;
  const char *s = src;
  if (s < d && d < s + n) {
    s += n;
    d += n;
    while (n--) {
      *--d = *--s;
    }
  } else {
    while (n--) {
      *d++ = *s++;
    }
  }
  return dst;
}

void *memcpy(void *out, const void *in, size_t n) {
  char *dst = out;
  const char *src = in;
  while (n--) {
    *dst++ = *src++;
  }
  return out;
}

int memcmp(const void *s1, const void *s2, size_t n) {
  const unsigned char *su1, *su2;
  for (su1 = s1, su2 = s2; 0 < n; ++su1, ++su2, --n) {
    if (*su1 != *su2) {
      return ((int)*su1) - ((int)*su2);
    }
  }
  return 0;
}

#endif
