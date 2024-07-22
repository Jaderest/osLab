#include "common.h"
#include "thread.h"

void do_test0() {
  printf("\033[44mTest 0: Running is successful "
         "2\033[0m\n");
}

int main() {
  do_test0();
  return 0;
}