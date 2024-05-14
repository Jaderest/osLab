#include <stdio.h>
#include <string.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/wait.h>

#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_CYAN "\x1b[36m"
#define ANSI_COLOR_RESET "\x1b[0m"

// #define DEBUG
#ifdef DEBUG
    #define debug(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
    #define debug(fmt, ...)
#endif

//! 禁止system和popen

void *handle[128];
int handle_len = 0;

typedef int (*func_ptr)();

char dir[] = "/tmp/crepl_XXXXXX";

int read_line(char *strin) {
    printf(ANSI_COLOR_GREEN "crepl> " ANSI_COLOR_RESET);
    char *ret = fgets(strin, 4096, stdin);
    if (ret == NULL) {
        return 0;
    }
    return 1;
}

#define FUNC 1
#define EXPR 2

void *compile(char *src, int id) { // 可以返回新创建文件的句柄，用于接下来的表达式计算
    char file_name[4096];
    char so_name[4096];
    snprintf(file_name, 4096, "%s/crepl%d.c", dir, handle_len);
    snprintf(so_name, 4096, "%s/lib%d.so", dir, handle_len);

    FILE *fp = fopen(file_name, "w");
    if (fp == NULL) {
        perror("fopen()");
        return NULL;
    }
    fprintf(fp, "%s", src);
    fclose(fp);

    char *argv[] = {"gcc", "-shared", "-fPIC", "-w", file_name, "-o", so_name, NULL};
    pid_t pid = fork();
    if (pid == 0) {
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
        execvp("gcc", argv);
        perror("execvp()");
        return NULL;
    } else {
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            handle[handle_len] = dlopen(so_name, RTLD_LAZY | RTLD_GLOBAL); // 无敌了RTLD_GLOBAL
            if (handle[handle_len] == NULL) {
                perror("dlopen()");
                return NULL;
            }
            handle_len++;
            if (id == FUNC) {
                printf(ANSI_COLOR_CYAN "Add: " ANSI_COLOR_RESET);
                printf("%s", src); // src自己背后会有一个换行的
            }
        } else {
            printf(ANSI_COLOR_RED "Compile error\n" ANSI_COLOR_RESET);
        }
    }

    return handle[handle_len - 1];
}

void calc_expr(char *text) { // 包一下
    char src[4096];
    char func_name[4096];

    snprintf(func_name, 4096, "__expr_wrap_%d", handle_len);
    int len = strlen(text);
    if (text[len - 1] == '\n') {
        text[len - 1] = '\0';
    }
    snprintf(src, 4096, "int __expr_wrap_%d() { return %s; }", handle_len, text);
    debug("src: %s\n", src);

    void *handle = compile(src, EXPR);
    debug("handle: %p\n", handle);
    if (handle != NULL) {
        debug("func_name: %s\n", func_name);

        // 进程间通信
        int pipefd[2];
        if (pipe(pipefd) == -1) {
            perror("pipe()");
            return;
        }
        // 创建子进程
        pid_t pid = fork();
        if (pid == -1) {
            perror("fork()");
            return;
        } else if (pid == 0) {
            close(pipefd[0]); // close read end
            func_ptr func = dlsym(handle, func_name);
            int ans = func();
            write(pipefd[1], &ans, sizeof(int));
            close(pipefd[1]);
            exit(0);
        } else {
            close(pipefd[1]); // close write end
            int ans;
            read(pipefd[0], &ans, sizeof(int));
            close(pipefd[0]);

            int status;
            waitpid(pid, &status, 0);
            if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
                printf(ANSI_COLOR_CYAN);
                printf("(%s)", text);
                printf(ANSI_COLOR_RESET);
                printf(" = ");
                printf("%d\n", ans);
            } else {
                printf(ANSI_COLOR_RED "Runtime error\n" ANSI_COLOR_RESET);
            }
        }
    }
}

void close_handle() {
    for (int i = 0; i < handle_len; i++) {
        dlclose(handle[i]);
    }
}

int main(int argc, char *argv[]) {
    static char line[4096];
    if (mkdtemp(dir) == NULL) { //创建临时目录
        perror("mkdtemp()");
        return 1;
    }

    while (1) {
        if (!read_line(line)) {
            perror("fgets()");
            break;
        }
        if (strcmp(line, "exit\n") == 0) {
            break;
        } else if (strncmp(line, "int ", 4) == 0) { //func
            debug("func\n");
            compile(line, FUNC);
        } else { //expr
            debug("expr\n");
            calc_expr(line);
        }
    }

    close_handle();
    return 0;
}