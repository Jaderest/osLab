#include "common.h"
#include "thread-sync.h"
#include "thread.h"
#include <assert.h>
#include <unistd.h>

static void entry(int tid) {pmm->alloc(128);}
static void goodbye() {printf("Goodbye, world\n");}

int main() {
  pmm->init();
  for (int i = 0; i < 4; i++) {
    create(entry);
  }
  join();
  return 0;
}