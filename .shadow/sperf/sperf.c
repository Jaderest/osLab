#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>
#include <unistd.h>
#include <fcntl.h>
#include <regex.h>

// #define DEBUG
#ifdef DEBUG
#define debug(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define debug(fmt, ...)
#endif

#define MAX_SYSCALL 2048

typedef struct {
    char name[256];
    double total_time;
} Syscall;

typedef struct {
    Syscall *data;
    size_t size;
    size_t capacity;
} SyscallArray;

void init_syscall_array(SyscallArray *arr) {
    arr->size = 0;
    arr->capacity = 10;
    arr->data = malloc(arr->capacity * sizeof(Syscall));
}

void free_syscall_array(SyscallArray *arr) {
    free(arr->data);
    arr->size = 0;
    arr->capacity = 10;
    arr->data = malloc(arr->capacity * sizeof(Syscall));
}

void add_syscall(SyscallArray *arr, const char *name, double time) {
    for (size_t i = 0; i < arr->size; ++i) {
        if (strcmp(arr->data[i].name, name) == 0) {
            (arr->data[i]).total_time += time;
            return;
        }
    }
    if (arr->size >= arr->capacity) {
        arr->capacity *= 2;
        arr->data = realloc(arr->data, arr->capacity * sizeof(Syscall));
    }
    assert(arr->size < arr->capacity);
    assert(arr->data); // 这里data assert失败了
    strcpy(arr->data[arr->size].name, name);
    arr->data[arr->size].total_time = time;
    arr->size++;
}

int compare_syscall(const void *a, const void *b) {
    double diff = ((Syscall *)b)->total_time - ((Syscall *)a)->total_time;
    return (diff > 0) - (diff < 0);
}

void print_top_syscalls(SyscallArray *arr, size_t n, double total) {
    qsort(arr->data, arr->size, sizeof(Syscall), compare_syscall);
    //TODO: 修改一下
    // printf("Top %zu system calls:\n", n);
    // printf("Total time: %f\n", total);
    printf("------------------------\n");
    for (size_t i = 0; i < n && i < arr->size; ++i) {
        int rate = (int)(arr->data[i].total_time / total * 100);
        printf("%s (%d%%)\n", arr->data[i].name, rate);
        for(int j = 0; j < 80; j++) {
            printf("%c", '\0');
        }
        fflush(stdout);
    }
}

// 子进程strace，持续通信给父进程输出相应信息，父进程创造数据结构存储系统调用时间，此时两个进程是并行的
// 父进程一定不能等子进程结束，二者必须要通信，然后父进程需要统计时间进行输出，子进程则负责环境变量的查找

int main(int argc, char *argv[], char *envp[]) { // 参数存在argv中
    /*-------prepare argvs-------*/
    for (int i = 0; i < argc; i++) {
        assert(argv[i]);
        debug("argv[%d] = %s\n", i, argv[i]);
    }
    assert(!argv[argc]);

    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe()");
        return 1;
    }
    pid_t pid = fork();

    if (pid == 0) {
        //子进程
        close(pipefd[0]); // close read end
        dup2(pipefd[1], STDERR_FILENO); // redirect stderr to pipe，即将strace的输出重定向过去
        //strace -T -ttt ../pstree/pstree-64 -V
        //记住这个例子，它会有标准错误输出，所以一股脑重定向过去会导致pstree的输出也会过去，从而导致读取错误，find /的结果也会过去
        int fd = open("/dev/null", O_WRONLY);
        dup2(fd, STDOUT_FILENO); // 不输出子进程的其他东西

        int strace_argc = 3 + (argc - 1) + 1;
        char *strace_argv[strace_argc];
        strace_argv[0] = "/usr/bin/strace";
        strace_argv[1] = "-T";
        strace_argv[2] = "-ttt";
        for (int i = 1; i < argc; i++) {
            strace_argv[i + 2] = argv[i];
        }
        strace_argv[strace_argc - 1] = NULL;
        for (int i = 0; i < strace_argc; i++) {
            debug("strace_argv[%d] = %s\n", i, strace_argv[i]);
        }
        execve("/usr/bin/strace", strace_argv, envp);
    } else if (pid > 0) {
        // 父进程
        close(pipefd[1]); // close write end
        // 数据结构
        SyscallArray syscall_array;
        init_syscall_array(&syscall_array);
        double st = 0.0f; //作为每一个节点统计的开始
        double total = 0.0f; //总时间
        
        //---------------正则表达式----------------
        regex_t regex;
        regmatch_t matchs[4];
        const char *pattern = "^([0-9]+\\.[0-9]+) ([a-zA-Z0-9_]+)\\(.*<([0-9]+\\.[0-9]+)>";
        if (regcomp(&regex, pattern, REG_EXTENDED) != 0) {
            perror("regcomp()");
            return 1;
        }
        char buf[1024];
        while (fgets(buf, sizeof(buf), fdopen(pipefd[0], "r")) != NULL) {
            // printf("%s", buf); // 奇怪啊
            if (regexec(&regex, buf, 4, matchs, 0) == 0) {
                buf[matchs[1].rm_eo] = '\0';
                buf[matchs[2].rm_eo] = '\0';
                buf[matchs[3].rm_eo] = '\0';

                double start_time = atof(buf + matchs[1].rm_so);
                const char *name = buf + matchs[2].rm_so;
                double syscall_time = atof(buf + matchs[3].rm_so);

                add_syscall(&syscall_array, name, syscall_time);
                total += syscall_time;
                // debug("%f  %s  %f\n", start_time, name, syscall_time);
                if (st == 0.0f) {
                    debug("ssssssst = %f\n", st);
                    st = start_time;
                } else if (start_time - st > 0.1f) {
                    debug("------------------------\n");
                    debug("st = %f\n", st);
                    print_top_syscalls(&syscall_array, 5, total);
                    debug("before free_syscall_array\n");
                    debug("after free_syscall_array\n");
                    st = 0.0f;
                }
            }
            // 在这个循环中不断读取输出
        }
        print_top_syscalls(&syscall_array, 5, total);
        free_syscall_array(&syscall_array);
        regfree(&regex);
    } else {
        perror("fork()");
        return 1;
    }

    return 0;
}
