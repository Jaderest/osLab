#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#define MAX_PROC 256
#define MAX_DEPTH 16

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
      sscanf(line, "Name: %[^\n]", process->name);
    }
  }
  
  fclose(status);
}

typedef struct PROCNODE {
  char name[256];
  int pid;
  int ppid;
  struct PROCNODE **children;
  int childCount;
} ProcNode;

ProcNode *creatNode(Process *p) { // initial
  ProcNode *node = (ProcNode*)malloc(sizeof(ProcNode));
  strcpy(node->name, p->name); // deep copy
  node->pid = p->pid;
  node->ppid = p->ppid;
  node->children = NULL;
  node->childCount = 0;
  return node;
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
  switch (depth) {
    case 0:
      break;
    case 1:
      printf("+-");
      break;
    default: {
      for (int i = 0; i < depth - 1; i++) printf("%s", blank[i]);
      printf("+-");
      break;
    }
  }
  printf("%s\n", root->name);

  for (int i = 0; i < root->childCount; i++) {
    if (i == root->childCount - 1) {
      strcpy(blank[i], "  \0");
    }
    printTree(root->children[i], depth + 1);
    if (i == root->childCount - 1) {
      for (int j = i; j < MAX_DEPTH; j++) {
        strcpy(blank[i], "| \0");
      }
    }
  }
}

void PrintTree(Process *process[], int count) {
  ProcNode *node[MAX_PROC];
  for (int i = 0; i < count; i++) {
    node[i] = creatNode(process[i]);
  }

  for (int i = 0; i < count; i++) { // proc1 must be the first
    findPPid(node[i], node, count);
  }

  for (int i = 0; i < MAX_DEPTH; i++) {
    strcpy(blank[i], "| \0");
  }
  printTree(node[0], 0);
}

int main(int argc, char *argv[]) {
  DIR *dir;
  struct dirent *entry;
  Process *process[MAX_PROC] = {NULL};
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
