#include "common.h"
#include "thread.h"

static inline size_t min(size_t a, size_t b) {
  return a < b ? a : b;
}
#define N 100010
#define PAGE_SIZE 4096
#define MAGIC 0x6666

int n, count = 0;

void test0() {
  printf("\033[44mTest 0: Check test frame"
         "\033[0m\n");
}

int main() {
  pmm->init();
  test0();
  printf("\033[42mPASSED\033[0m\n");
}