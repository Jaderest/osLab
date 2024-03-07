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
  char name[256];
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

void parsePid(const char *statusPath, Process *process) {
  FILE *status = fopen(statusPath, "r");
  char line[256];
  
  while (fgets(line, sizeof(line), status) != NULL) {
    //finish!
    if (strstr(line, "Pid:") != NULL) {
      sscanf(line, "Pid: %d", &process->pid);
    }
    if (strstr(line, "PPid:") != NULL) {
      sscanf(line, "PPid: %d", &process->ppid);
    }
    if (strstr(line, "Name:") != NULL) {
      //TODO: copy the name of process
      sscanf(line, "Name: %[^\n]", process->name);
    }
  }
  
  fclose(status);
}

void printTree(Process *process[], int count) {
  //TODO: finish building the tree
  printf("%s:%d %d\n", process[0]->name, process[0]->pid, process[0]->ppid);
  printf("%s:%d %d\n", process[1]->name, process[1]->pid, process[1]->ppid);
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
      
      //path to status
      char statusPath[512];
      snprintf(statusPath, sizeof(statusPath), "/proc/%s/status", entry->d_name);

      process[count] = malloc(sizeof(Process));
      process[count]->pid = 0;
      process[count]->ppid = 0;
      parsePid(statusPath, process[count]);

      count++;
    }
    if (count >= MAX_DIRS) {
      fprintf(stderr, "Too many directoris\n");
      break;
    }
  }

  closedir(dir);

  printTree(process, count);

  for (int i = 0; i < count; i++) {
    if (process[i] != NULL) {
      free(process[i]);
    }
  }

  return 0;
}
