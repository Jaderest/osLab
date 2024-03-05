#include <stdio.h>
#include <assert.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  for (int i = 0; i < argc; i++) {
    assert(argv[i]);
    printf("argv[%d] = %s\n", i, argv[i]);
  }
  pid_t p = getpid();
  printf("%d\n", p);
  assert(!argv[argc]);
  return 0;
}
