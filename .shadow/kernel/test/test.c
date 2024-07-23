#include "common.h"
#include "thread.h"

void test0() {
  printf("\033[44mTest 0: Check test frame"
         "\033[0m\n");
  pmm->alloc(1);
}

int main() {
  pmm->init();
  test0();
  printf("\033[42mPASSED\033[0m\n");
}