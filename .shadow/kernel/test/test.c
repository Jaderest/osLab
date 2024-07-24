#include "common.h"
#include "thread.h"

static inline size_t min(size_t a, size_t b) { return a < b ? a : b; }
#define N 100010
#define PAGE_SIZE 4096
#define MAGIC 0x6666

int n, count = 0;             // 缓冲区大小
#define MAX_MEM_SZ (96 << 20) // 96MB

size_t total_sz = 0;
mutex_t lk = MUTEX_INIT();
cond_t cv = COND_INIT();

size_t alloc_random_sz() {
  int percent = rand() % 100;
  if (percent == 0) {
    return (rand() % 1024 + 1) * PAGE_SIZE;
  } else if (percent <= 10) {
    return (rand() % (4096 - 128) + 129);
  } else {
    return rand() % 128 + 1;
  }
}

void align_check(void *p, size_t sz) {
  if (sz == 1)
    return;
  for (int i = 1; i < 64;
       i++) { // 遍历这么多位，地址要满足2^i的对齐，即最后%1 << i == 0
    if (sz > (1 << (i - 1)) && sz <= (1 << i)) {
      PANIC_ON((uintptr_t)p % (1 << i) != 0, "Alignment check failed: %p %d\n",
               p, sz);
      return;
    }
  }
}

void double_alloc_check(void *ptr, size_t size) {
  unsigned int *arr = (unsigned int *)ptr;
  for (int i = 0; (i + 1) * sizeof(unsigned int) <= size; i++) {
    PANIC_ON(arr[i] != MAGIC, "Double alloc check failed: %p %ld\n", ptr, size);
    arr[i] = MAGIC;
  }
}

void clear_magic(void *ptr, size_t size) {
  unsigned int *arr = (unsigned int *)ptr;
  for (int i = 0; (i + 1) * sizeof(unsigned int) <= size; i++) {
    arr[i] = 0;
  }
}

void test0() {
  printf("\033[44mTest 0: Check alignment"
         "\033[0m\n");
  for (int i = 0; i < 100000; i++) {
    int sz = alloc_random_sz() % 2048 + 1; //[1, 2048]
    void *ret = pmm->alloc(sz);

    printf("alloc %d bytes at %p\n", sz, ret);

    double_alloc_check(ret, sz);

    printf("Double alloc check\n");

    align_check(ret, sz);

    printf("Alignment check\n");

    pmm->free(ret);
    clear_magic(ret, sz);
  }
}

int main() {
  pmm->init();
  test0();
  printf("\033[42mPASSED\033[0m\n");
}