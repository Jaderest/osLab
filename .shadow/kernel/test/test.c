#include "common.h"
#include "threads.h"
#include <assert.h>
#include <sys/syscall.h>
#include <unistd.h>

static void entry0(int tid) { pmm->alloc(128); }
static void goodbye()       { printf("End.\n"); }
int main() {
  pmm->init();
  for (int i = 0; i < 4; i++) {
    create(entry0);
  }
  join(goodbye);
}