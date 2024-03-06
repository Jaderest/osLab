#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define MAX_DIRS 256

int main(int argc, char *argv[]) {
  for (int i = 0; i < argc; i++) {
    assert(argv[i]);
    printf("argv[%d] = %s\n", i, argv[i]);
  }
  pid_t p = getpid();
  printf("%d\n", p);

  DIR *dir;
  struct dirent *entry;
  char *dirs[MAX_DIRS];
  int count = 0;

  // open the directory
  dir = opendir("/proc");
  if (dir == NULL) {
    perror("Error: Unable to open directory");
    return 1;
  }
  while((entry = readdir(dir)) != NULL) {
    if (entry->d_type == DT_DIR && entry->d_name[0] != '.') { // ignore. && ..
      dirs[count] = malloc(strlen(entry->d_name) + 1);
      strcpy(dirs[count], entry->d_name);
      count++;
    }
    if (count >= MAX_DIRS) {
      fprintf(stderr, "Too many directoris\n");
      break;
    }
  }

  closedir(dir);

  for (int i = 0; i < count; i++) {
    printf("%s ", dirs[i]);
    free(dirs[i]);
  }

  printf("\n");

  assert(!argv[argc]);
  return 0;
}
