#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#define MAX_PROC 256

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

void printTree(ProcNode* root, int depth) { //recursion
  for (int i = 0; i < depth; i++) {
    printf("  ");
  }
  printf("%s\n", root->name);

  for (int i = 0; i < root->childCount; i++) {
    printTree(root->children[i], depth + 1);
  }
}

void PrintTree(Process *process[], int count) {
  //TODO: finish building the tree
  ProcNode *node[MAX_PROC];
  for (int i = 0; i < count; i++) {
    // printf("%s: pid = %d, ppid = %d\n", process[i]->name, process[i]->pid, process[i]->ppid);
    node[i] = creatNode(process[i]);
  }

  for (int i = 1; i < count; i++) { // 默认进程 1 是第一个读出来的了，所以找node[0]的ppid，但是找了也没事，只是浪费时间
    findPPid(node[i], node, count);
  }

  for (int i = 0; i < count; i++) {
    printf("%s: pid = %d\n", node[i]->name, node[i]->pid);
  }
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
