#include <am.h>
#include <klib.h>
#include <klib-macros.h>

//以下行表示接入标准库或者老师实现的klib，不然就用自己实现的
#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)
static unsigned long int next = 1;

typedef struct memory_block {
  size_t size;
  struct memory_block *next;
} mem_block;
// 空闲内存链表头指针
static mem_block *free_list = NULL;


int rand(void) {
  // RAND_MAX assumed to be 32767
  next = next * 1103515245 + 12345;
  return (unsigned int)(next/65536) % 32768;
}

void srand(unsigned int seed) {
  next = seed;
}

int abs(int x) {
  return (x < 0 ? -x : x);
}

int atoi(const char* nptr) {
  int x = 0;
  while (*nptr == ' ') { nptr ++; }
  while (*nptr >= '0' && *nptr <= '9') {
    x = x * 10 + *nptr - '0';
    nptr ++;
  }
  return x;
}

void *malloc(size_t size) {
  // On native, malloc() will be called during initializaion of C runtime.
  // Therefore do not call panic() here, else it will yield a dead recursion:
  //   panic() -> putchar() -> (glibc) -> malloc() -> panic()
#if !(defined(__ISA_NATIVE__) && defined(__NATIVE_USE_KLIB__))
  mem_block *current, *prev;
  void *result;

  // 对齐内存块大小
  size = (size + sizeof(mem_block) - 1) / sizeof(mem_block) * sizeof(mem_block);

  for (current = free_list, prev = NULL; current != NULL; prev = current, current = current->next) {
    if (current->size >= size) {
      if (current->size > size + sizeof(mem_block)) { // 找到足够大的内存块
        mem_block *split_block = (mem_block *)((char *)current + size);
        split_block->size = current->size - size - sizeof(mem_block);
        split_block->next = current->next;
        if (prev == NULL) {
          free_list = split_block;
        } else {
          prev->next = split_block;
        }
      } else { // 使用整个内存块
        if (prev == NULL) {
          free_list = current->next;
        } else {
          prev->next = current->next;
        }
      }
      result = (void *)((char *)current + sizeof(mem_block));
      return result;
    }
    return NULL;
  }
#endif
  return NULL;
}

void free(void *ptr) {
  if (ptr == NULL) {
    return;
  }

  mem_block *block = (mem_block *)((char *)ptr - sizeof(mem_block));

  block->next = free_list;
  free_list = block;
}

#endif
