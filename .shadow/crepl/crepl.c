#include <stdio.h>
#include <string.h>
#include <dlfcn.h>

void *handle[128]; // 用于存储静态库
int handle_len = 0;



int main(int argc, char *argv[]) {
    static char line[4096];

    while (1) {
        printf("crepl> ");
        fflush(stdout);

        if (!fgets(line, sizeof(line), stdin)) {
            break;
        }
        char line_copy[4096];

        strcpy(line_copy, line);
        char *cmd = strtok(line_copy, " ");
        if (strcmp(cmd, "int") == 0) {
            printf("int\n");
        } else {
            printf("expr_wrapper\n");
        }

        // To be implemented.
        printf("Got %zu chars.\n", strlen(line));
    }
}
