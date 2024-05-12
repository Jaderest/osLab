#include <stdio.h>
#include <string.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

//! 禁止system和popen

void *handle[128]; // 用于存储静态库
int handle_len = 0;

int create_handle(char *filename, char *text) { // text为用户输入的代码
    // TODO: create a file, gcc and dlopen it. 
    // char *args[] = {
    //     "gcc",
    //     "-shared",
    //     "-fPIC",
    //     "-o",
    //     filename,
    //     NULL
    // };
    char *args[] = {
        "ls",
        "-l",
        NULL
    };
    pid_t pid = fork();
    if (pid == 0) {
        execv("/usr/bin/ls", args); //! 当前进程会在执行完execv后退出！所以需要fork后再执行
        perror("execv");
        return(1);
    } else if (pid > 0) {
        waitpid(pid, NULL, 0);
        printf("Child exited\n");
    } else {
        perror("fork");
        return(1);
    }

    return 0;
}

void close_handle() {
    for (int i = 0; i < handle_len; i++) {
        dlclose(handle[i]);
    }
}

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

        char *token = strtok(line_copy, " ");
        if (strcmp(token, "int") == 0) {
            token += 4;
            // TODO create a file, gcc and dlopen it.
            create_handle("/tmp/tmp.c", token);
            // printf("Add int %s", token);
        } else {
            // TODO same
            printf("expr_wrapper: %s", line);
        }

    }

    close_handle();
    return 0;
}
