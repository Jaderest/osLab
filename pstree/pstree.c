#include <stdio.h>

#define MAX_PROC 512

typedef struct PROCESS {
  char name[128];
  int pid; // pid
  int ppid; // parents pid
  struct PROCESS **children;
  int childCount;
} Process;

int _p = 0;
int _n = 0;
int _v = 0;

int isNumeric(const char *str) {
  while(*str) {
    if (!isdigit(*str)) {
      return 0;
    }
    str++;
  }
  return 1;
} //and now i know why my git have so many bug

void parsePid(const char *statusPath, ProcNode *process) {
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
      sscanf(line, "Name: %[^\n]", process->name);
    }
  }
  
  fclose(status);
}

void findPPid(ProcNode *child, ProcNode *nodes[], int count) {
  for (int i = 0; i < count; i++) {
    if (child->ppid == nodes[i]->pid) {
      nodes[i]->children = (ProcNode**)realloc(nodes[i]->children, (nodes[i]->childCount + 1) * sizeof(ProcNode*));
      nodes[i]->children[nodes[i]->childCount++] = child;
      return;
    }
  }
}

char blank[MAX_DEPTH][3];

void printTree(ProcNode* root, int depth) { //recursion
  for (int i = 0; i < depth; i++) {
    printf("  ");
  }
  printf("%s\n", root->name);

  for (int i = 0; i < root->childCount; i++) {
    printTree(root->children[i], depth + 1);
  }
}

void PrintTree(ProcNode *process[], int count) {
  for (int i = 0; i < count; i++) { // proc1 must be the first
    findPPid(process[i], process, count);
  }

  printTree(process[0], 0);
}

int main(int argc, char *argv[]) {
  DIR *dir;
  struct dirent *entry;
  ProcNode *process[MAX_PROC] = {NULL};
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
      process[count]->children = NULL;
      process[count]->childCount = 0;

      parsePid(statusPath, process[count]);

      count++;
    }
    if (count >= MAX_PROC) {
      fprintf(stderr, "Too many directoris\n");
      break;
    }
  }

  closedir(dir);

  PrintTree(process, count);

  for (int i = 0; i < count; i++) {
    if (process[i] != NULL) {
      free(process[i]);
    }
  }

  return 0;
}