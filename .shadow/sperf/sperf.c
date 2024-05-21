#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>

#define DEBUG
#ifdef DEBUG
#define debug(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define debug(fmt, ...)
#endif

#define MAX_CALLS 1024

typedef struct {
    char name[64];
    double total_time;
    int count;
} syscall_info_t;

syscall_info_t syscalls[MAX_CALLS];
int syscalls_num = 0;

void add_syscall_time(const char *name, double time) {
    for (int i = 0; i < syscalls_num; i++) {
        if (strcmp(syscalls[i].name, name) == 0) {
            syscalls[i].total_time += time;
            syscalls[i].count++;
            return;
        }
    }
    assert(syscalls_num < MAX_CALLS);
    strcpy(syscalls[syscalls_num].name, name);
    syscalls[syscalls_num].total_time = time;
    syscalls[syscalls_num].count = 1;
    syscalls_num++;
}

regex_t reg;
regmatch_t matches[4];
int deal_line(char *line) {
    if (syscalls_num == 0) { // 还没有编译
        const char *pattern = "^([a-zA-Z0-9_]+)\\(.*\\)\\s+=\\s+.*\\s+<([0-9.]+)>";
        if (regcomp(&reg, pattern, REG_EXTENDED) == 0) {
            debug("Compile regex: %s\n", pattern);
        } else {
            debug("Compile regex failed: %s\n", pattern);
            return -1;
        }
    }

    if (regexec(&reg, line, 4, matches, 0) == 0) { // 正则表达式
        char name[64];
        double time;
        strncpy(name, line + matches[1].rm_so, matches[1].rm_eo - matches[1].rm_so);
        name[matches[1].rm_eo - matches[1].rm_so] = '\0';
        time = atof(line + matches[2].rm_so);
        add_syscall_time(name, time);
    }
    return 0;
}

void close_reg() {
    regfree(&reg);
}

int cmp_syscalls(const void *a, const void *b) {
    syscall_info_t *pa = (syscall_info_t *)a;
    syscall_info_t *pb = (syscall_info_t *)b;

    if (pa->total_time > pb->total_time) return -1;
    if (pa->total_time < pb->total_time) return 1;
    return 0;
}

void show_syscalls() {
    system("clear");
    qsort(syscalls, syscalls_num, sizeof(syscall_info_t), cmp_syscalls);
    double all_time = 0;
    for (int i = 0; i < syscalls_num; ++i) {
        all_time += syscalls[i].total_time;
    }
    int min = syscalls_num > 5 ? 5 : syscalls_num;
    for (int i = 0; i < min; ++i) {
        int ratio = (int)((syscalls[i].total_time / all_time) * 100);
        printf("%s (%d%%)\n", syscalls[i].name, ratio);
        for (int j = 0; j < 80; ++j) {
            printf("%c", '\0');
        }
    }
}

void show_verbose_syscalls() {
    qsort(syscalls, syscalls_num, sizeof(syscall_info_t), cmp_syscalls);
    for (int i = 0; i < syscalls_num; ++i) {
        printf("%-20s %-10.6f %-10d\n", syscalls[i].name, syscalls[i].total_time, syscalls[i].count);
    }
}

int main(int argc, char *argv[], char *envp[]) {
    for (int i = 0; i < argc; i++) {
        assert(argv[i]);
        debug("argv[%d] = %s\n", i, argv[i]);
    }
    assert(!argv[argc]);
    
    FILE *fp = fopen("stra.txt", "r");
    assert(fp); // 随便检测一下
    char line[1024];
    while (fgets(line, sizeof(line), fp)) {
        deal_line(line);
    }

    // show_syscalls();
    show_verbose_syscalls();

    close_reg();
    fclose(fp);
    return 0;
}



// 仅用execve
// 每次输出top 5的系统调用、每个系统调用至多输出一次
/*
TODO：2. strace  -T 输出格式，我们将argv保存起来，传给strace
TODO：3. 如何查找环境变量
TODO：4. 父进程不断读取子进程的输出，直到strace结束（进程需要保持实时通信）
子进程是要使用strace的，父进程读取strace输出，现在理一下逻辑
传进来参数，
1. 父进程创建子进程
2. 子进程execve执行strace，这里的ARG是strace执行将要执行程序的参数
3. 父进程不断读取子进程的输出，直到strace结束
子进程：
    execve strace
    pipe传递信息（中间的阻塞仔细想想）
父进程：
    创建子进程
    读取子进程的输出
    解析输出
    输出结果（时间间隔、排序）
*/
/*
int main(int argc, char *argv[]) { // 参数存在argv中
    for (int i = 0; i < argc; i++) {
        assert(argv[i]);
        printf("argv[%d] = %s\n", i, argv[i]);
    }
    assert(!argv[argc]);
    return 0;
}
*/
