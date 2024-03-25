#include <stdio.h>
#include <assert.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define MAX_PROC 2048
#define MAX_DEPTH 1024

typedef struct PROCESS {
  char name[128];
  int pid;
  int ppid;
  struct PROCESS **children;
  int childCount;
} Process;

int _p = 0;
int _n = 0;
int _v = 0;
int type[MAX_DEPTH]; // 0:└── , 1:├──

int isNumeric(const char *str) {
  while(*str) {
    if(*str < '0' || *str > '9') {
      return 0;
    }
    str++;
  }
  return 2;
}

void parseProcess(const char *statusPath, Process *process) {
  FILE *statusFile = fopen(statusPath, "r");
  if(statusFile == NULL) {
    perror("Unable to open status file");
    return;
  }

  char line[256];
  while(fgets(line, sizeof(line), statusFile)) {
    if (strstr(line, "Pid:") != NULL) {
      sscanf(line, "Pid: %d", &process->pid);
    }
    if (strstr(line, "PPid:") != NULL) {
      sscanf(line, "PPid: %d", &process->ppid);
    }
    if (strstr(line, "Name:") != NULL) {
      sscanf(line, "Name: %[^\n]", process->name);
    }
  }

  fclose(statusFile);
}

void findPPid(Process *child, Process *nodes[], int count) {
  for (int i = 0; i < count; i++) {
    if (child->ppid == nodes[i]->pid) {
      nodes[i]->children = (Process **)realloc(nodes[i]->children, (nodes[i]->childCount + 1) * sizeof(Process));
      nodes[i]->children[nodes[i]->childCount] = child;
      nodes[i]->childCount++;
      return;
    }
  }
}

void printTree(Process *root, int depth, int is_last) {
  for (int i = 0; i < depth - 1; i++) {
    if (type[i] == 0) printf("│  ");
    else printf("   ");
  }
  if (depth > 0) {
    if (is_last) {
      printf("└──");
      type[depth - 1] = 1;
    } else {
      printf("├──");
      type[depth - 1] = 0;
    }
  }
  printf("%s\n", root->name);

  for (int i = 0; i < root->childCount; i++) {
    printTree(root->children[i], depth + 1, i == root->childCount - 1);
  }
}

int compare(const void *a, const void *b) {
  return strcmp((*(Process **)a)->name, (*(Process **)b)->name);
}

void PrintTree(Process *process[], int count) {
  for (int i = 0; i < count; i++) { // proc1 must be the first
    findPPid(process[i], process, count);
  }

  if (_p == 1) {
    for (int i = 0; i < count; i++) {
      char new_name[256];
      snprintf(new_name, sizeof(new_name), "%s(%d)", process[i]->name, process[i]->pid);
      strcpy(process[i]->name, new_name);
    }
  }

  if (_n == 1) ;
  else { //sort by name
    for (int i = 0; i < count; i++) {
      qsort(process[i]->children, process[i]->childCount, sizeof(Process *), compare);
    }
  }

  for (int i = 0; i < MAX_DEPTH; i++) { type[i] = 0; }
  printTree(process[0], 0, 1);
}

int main(int argc, char *argv[]) {
  assert(argv[0]);
  for (int i = 1; i < argc; i++) {
    assert(argv[i]); // C 标准保证
    if (strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--show-pids") == 0) {
      _p = 1;
    } else if (strcmp(argv[i], "-n") == 0 || strcmp(argv[i], "-numeric-sort") == 0) {
      _n = 1;
    } else if (strcmp(argv[i], "-V") == 0 || strcmp(argv[i], "--version") == 0) {
      _v = 1;
    } else {
      perror("Args error");
      return 1;
    }
  }
  assert(!argv[argc]); // C 标准保证

  if (_v == 1) {
    printf("pstree 1.1\n");
    printf("This is a minilab finished by Jaderest\n");
    printf("He is really happy to do this as homework in OS class\n");
    printf("He has finished beautifying the tree\n");
    return 0;
  }

  DIR *dir;
  struct dirent *entry;
  Process *process[MAX_PROC] = {NULL};
  int count = 0;

  dir = opendir("/proc");
  if(dir == NULL) {
    perror("Unable to open /proc");
    return 2;
  }

  while((entry = readdir(dir)) != NULL) {
    if(entry->d_type == DT_DIR && entry->d_name[0] != '.' && isNumeric(entry->d_name)) {

      char statusPath[512];
      snprintf(statusPath, sizeof(statusPath), "/proc/%s/status", entry->d_name);

      process[count] = malloc(sizeof(Process));
      process[count]->pid = 0;
      process[count]->ppid = 0;
      process[count]->childCount = 0;
      process[count]->children = NULL;
      parseProcess(statusPath, process[count]);
      count++;
    }
  }

  closedir(dir);

  PrintTree(process, count);

  for (int i = 0; i < count; i++) {
    free(process[i]);
  }
  return 0;
}