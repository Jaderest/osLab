#include <stdio.h>
#include <string.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

//! 禁止system和popen

#ifdef DEBUG
#define debug(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define debug(fmt, ...)
#endif

void *handle[128]; // 用于存储静态库
int handle_len = 0;

int use_gcc(char src[], char dst[]) {
    return 0;
}

int create_src(char file[], char file_name[], char text[]) {
    int fd = mkstemp(file); // 包含了路径
    if (fd == -1) {
        debug("mkstemp\n");
        return 1;
    }
    if (rename(file, file_name) == -1) {
        debug("rename\n");
        return 1;
    }
    FILE *fp = fopen(file_name, "w");
    if (fp == NULL) {
        debug("fopen\n");
        return 1;
    }

    fprintf(fp, "%s", text);
    return 0;
}

int load_handle(char *text) { // text为用户输入的代码，我们需要解析出函数名
    // TODO: 创建并加载
    // 创建文本(mkstemp + write + rename)
    // 编译一下，编译错误返回一个错误信息
    // dlopen，修改len和handle

    /*-------names-------*/
    char tmpfile[] = "/tmp/XXXXXX"; // 用于.c和.so文件
    char tmpfile_name[128]; // 用于存储文件名，加上.c后缀
    char tmpfile_so[128]; // 用于存储文件名，加上.so后缀
    strcpy(tmpfile_name, tmpfile);
    strcat(tmpfile_name, ".c");
    strcpy(tmpfile_so, tmpfile);
    strcat(tmpfile_so, ".so");

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
            debug("int: %s", token);
        } else { // "int"会识别成这里诶
            // TODO same
            debug("not int: %s", token);
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
