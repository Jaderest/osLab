#include <stdio.h>
#include <string.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <fcntl.h>
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

int create_file(char *file, char *file_name, char *file_so, char *text) { //ok!
    char src[128];
    strcpy(src, file);
    int fd = mkstemp(src); // 包含了路径
    if (fd == -1) {
        perror("mkstemp\n");
        return 1;
    }
    char src_name[128];
    char src_so[128];
    strcpy(src_name, src);
    strcpy(src_so, src);
    strcat(src_name, ".c");
    strcat(src_so, ".so");
    if (rename(src, src_name) == -1) {
        perror("rename\n");
        return 1;
    }
    close(fd);

    strcpy(file_name, src_name);
    strcpy(file_so, src_so);
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

int use_gcc(char *file_name, char *so_name) {
    debug("filename: %s\n", file_name);
    debug("so_name: %s\n", so_name);
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork\n");
        return 1;
    }

    if (pid == 0) {
        char *argv[] = {"gcc", "-shared", "-fPIC", file_name, "-o", so_name, NULL};

        // 保存编译结果并重定向到文件
        int compile_log = open("/tmp/compile_log.txt", O_CREAT | O_WRONLY | O_APPEND, 0644);
        if (compile_log == -1) {
            perror("open\n");
            return 1;
        }
        // gcc全部输出重定向到文件
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
        dup2(compile_log, STDOUT_FILENO);
        dup2(compile_log, STDERR_FILENO);

        execvp("gcc", argv);
        perror("execvp\n");
        return 1;
    } else {
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            char line[4096];
            FILE *file = fopen(file_name, "r");
            if (file == NULL) {
                perror("fopen\n");
                return 1;
            }
            fgets(line, 4096, file);
            fclose(file);
        } else {
            printf("Compile Error\n");
        }
    }
    return 0;
}

#define FUNC 1
#define EXPR 2

int load_handle(char *text, int id) { // id是不同调用的两种情况，FUNC和EXPR
    /*-------names-------*/
    char tmpfile[] = "/tmp/XXXXXX"; // 用于.c和.so文件
    char tmpfile_name[128]; // 用于存储文件名，加上.c后缀
    char tmpfile_so[128]; // 用于存储文件名，加上.so后缀
    strcpy(tmpfile_so, tmpfile);
    strcat(tmpfile_so, ".so");

    /*-------create_file-------*/
    if (create_file(tmpfile, tmpfile_name, tmpfile_so, text) != 0) {
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

    /*-------dlopen-------*/
    handle[handle_len] = dlopen(tmpfile_so, RTLD_LAZY);
    if (!handle[handle_len]) {
        fprintf(stderr, "%s\n", dlerror());
        return 1;
    }
    handle_len++;
    dlerror();
    FILE *file = fopen(tmpfile_name, "r");
    if (file == NULL) {
        perror("fopen\n");
        return 1;
    }
    char line[4096];
    fgets(line, 4096, file);
    if (id == FUNC) {
        printf("Add %s", line);
    }
    fclose(file);

    if (id == EXPR) {
        //TODO: 解析所有的函数，加载一下
        // 所以这里困难的是怎么调用表达式中的函数
        // 要获取函数指针
        int (*func)() = dlsym(handle[handle_len - 1], "__expr_warpper");
        char *error;
        debug("func\n");
        if ((error = dlerror()) != NULL) {
            printf("%s\n", error);
            return 0;
        }
        debug("func2\n");
        printf("%d\n", func()); // 所以这里调用出现问题了，y是未加载的函数
        //! 这里的原因是我没有加载好y，所以调用func()时候崩溃
        // 我要解决的是：如何识别到y，如何在真正没有y的时候不让它崩溃，我是否需要fork和exec？先不做了cnm
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
        } else { // "int"会识别成这里诶
            load_handle(line, EXPR);
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
