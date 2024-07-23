#include "common.h"
#include "thread.h"

void test0() {
  printf("Hello from test0\n");

}

int main() {
  pmm->init();
  test0();
}