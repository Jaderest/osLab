#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>

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

int main() {
    FILE *fp = fopen("stra.txt", "r");
    assert(fp); // 随便检测一下
    char line[1024];

    regex_t reg;
    regmatch_t matches[4];
    const char *pattern = "^([a-zA-Z0-9_]+)\\(.*\\)\\s+=\\s+.*\\s+<([0-9.]+)>";
    regcomp(&reg, pattern, REG_EXTENDED);
    
    while (fgets(line, sizeof(line), fp)) {
        if (regexec(&reg, line, 4, matches, 0) == 0) {
            char name[64];
            double time;
            strncpy(name, line + matches[1].rm_so, matches[1].rm_eo - matches[1].rm_so);
            name[matches[1].rm_eo - matches[1].rm_so] = '\0';
            time = atof(line + matches[2].rm_so);
            add_syscall_time(name, time);
        }
    }
    printf("%-20s %-10s %-10s\n", "Syscall", "Time (s)", "Count");
    for (int i = 0; i < syscalls_num; ++i) {
        printf("%-20s %-10.6f %-10d\n", syscalls[i].name, syscalls[i].total_time, syscalls[i].count);
    }

    regfree(&reg);
    fclose(fp);
    return 0;
}



// 仅用execve
// 每次输出top 5的系统调用、每个系统调用至多输出一次
/*
TODO：1. 正则表达式怎么用
TODO：2. strace  -T 输出格式，我们将argv保存起来，传给strace
TODO：3. 如何查找环境变量
TODO：4. 父进程不断读取子进程的输出，直到strace结束（进程需要保持实时通信）
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
