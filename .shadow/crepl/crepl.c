#include <stdio.h>
#include <string.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <sys/wait.h>

//! 禁止system和popen

#define DEBUG

#ifdef DEBUG
#define debug(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define debug(fmt, ...)
#endif

void *handle[128]; // 用于存储静态库
int handle_len = 0;

int create_file(char *file, char *file_name, char *text) { //ok!
    char src[128];
    strcpy(src, file);
    int fd = mkstemp(src); // 包含了路径
    if (fd == -1) {
        perror("mkstemp\n");
        return 1;
    }
    char src_name[128];
    strcpy(src_name, src);
    strcat(src_name, ".c");
    if (rename(src, src_name) == -1) {
        perror("rename\n");
        return 1;
    }
    close(fd);

    strcpy(file_name, src_name);
    return 0;
}

int write_src(char *filename, char *text) {
    FILE *fp = fopen(filename, "w");
    if (fp == NULL) {
        perror("fopen\n");
        return 1;
    }

    fprintf(fp, "%s", text);
    fclose(fp);
    return 0;
}

int write_expr(char *filename, char *text) {
    FILE *fp = fopen(filename, "a");
    if (fp == NULL) {
        perror("fopen\n");
        return 1;
    }

    int len = strlen(text);
    if (text[len - 1] == '\n') {
        text[len - 1] = '\0';
    }
    char expr[4096] = "int __expr_warpper() { return ";
    strcat(expr, text);
    strcat(expr, "; }");

    fprintf(fp, "%s", expr);
    fclose(fp);
    return 0;
}

#define FUNC 1
#define EXPR 2

int use_gcc(char *filename, char *so_name) {
    return 0;
}

int load_handle(char *text, int id) { // id是不同调用的两种情况，FUNC和EXPR
    /*-------names-------*/
    char tmpfile[] = "/tmp/XXXXXX"; // 用于.c和.so文件
    char tmpfile_name[128]; // 用于存储文件名，加上.c后缀
    char tmpfile_so[128]; // 用于存储文件名，加上.so后缀

    /*-------create_file-------*/
    if (create_file(tmpfile, tmpfile_name, text) != 0) {
        perror("create_file");
        return 1;
    }

    /*-------write_code-------*/
    if (id == FUNC) {
        if (write_src(tmpfile_name, text) != 0) {
            perror("write_src\n");
            return 1;
        }
    } else if (id == EXPR) {
        if (write_expr(tmpfile_name, text) != 0) {
            perror("write_expr\n");
            return 1;
        }
    } else {
        assert(0);
    }

    /*-------use_gcc-------*/
    if (use_gcc(tmpfile_name, tmpfile_so) != 0) {
        debug("use_gcc\n");
        return 1;
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
            load_handle(line, FUNC);
            debug("%s", line);
        } else { // "int"会识别成这里诶
            load_handle(line, EXPR);
            debug("expr: %s\n", line);
        }

    }

    close_handle();
    return 0;
}

/*
!获取foo函数的地址
// 打开共享库
handle = dlopen("libfoo.so", RTLD_LAZY);
if (!handle) {
    fprintf(stderr, "%s\n", dlerror());
    return 1;
}

// 清除现有的错误
dlerror();

// 获取foo函数的地址
提供句柄和函数名即可，所以我解析变量需要解析出函数名
*(void **) (&foo) = dlsym(handle, "foo");
if ((error = dlerror()) != NULL)  {
    fprintf(stderr, "%s\n", error);
    dlclose(handle);
    return 1;
}
*/
