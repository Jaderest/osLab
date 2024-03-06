#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#define MAX_DIRS 256

typedef struct PROCESS {
  int pid; // pid
  int ppid; // parents pid
} Process;

int isNumeric(const char *str) {
  while(*str) {
    if (!isdigit(*str)) {
      return 0;
    }
    str++;
  }
  return 1;
}

int main(int argc, char *argv[]) {
  DIR *dir;
  struct dirent *entry;
  Process *process[MAX_DIRS] = {NULL};
  int count = 0;

  // open the directory
  dir = opendir("/proc");
  if (dir == NULL) {
    perror("Error: Unable to open directory");
    return 1;
  }
  while((entry = readdir(dir)) != NULL) {
    if (entry->d_type == DT_DIR && entry->d_name[0] != '.' && isNumeric(entry->d_name)) { // ignore. && ..
      //TODO: load its pid and ppid
      //path to status
      char statusPath[512];
      snprintf(statusPath, sizeof(statusPath), "/proc/%s/status", entry->d_name);

      FILE *status = NULL;
      status = fopen(statusPath, "r");
      if (status == NULL) {
        perror("Error: Unable to open status file");
        continue;
      }

      int pid = 0;
      int ppid = 0;
      fscanf(status, "Pid: %d\nPPid: %d", &pid, &ppid);
      printf("%d %d\n", pid, ppid);
      fclose(status);

      process[count] = malloc(sizeof(Process));
      process[count]->pid = pid;
      process[count]->ppid = ppid;

      count++;
    }
    if (count >= MAX_DIRS) {
      fprintf(stderr, "Too many directoris\n");
      break;
    }
  }

  closedir(dir);

  for (int i = 0; i < count; i++) {
    // printf("%d %d\n", process[i]->pid, process[i]->ppid);
    if (process[i] != NULL) {
      free(process[i]);
    }
  }

  return 0;
}
