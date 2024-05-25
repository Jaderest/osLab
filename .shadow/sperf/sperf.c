#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>
#include <unistd.h>

// #define DEBUG
#ifdef DEBUG
#define debug(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define debug(fmt, ...)
#endif

#define MAX_SYSCALL 2048

typedef struct {
    double start_time;
    char name[64];
    double duration;
} syscall_info_t;

typedef struct {
    double total_time;
    char name[64];
    int count;
} syscall_summary_t;

int cmp(const void *a, const void *b) {
    return ((syscall_summary_t *)a)->total_time < ((syscall_summary_t *)b)->total_time;
}

int main(int argc, char *argv[], char *envp[]) { // 参数存在argv中
    /*-------prepare argvs-------*/
    for (int i = 0; i < argc; i++) {
        assert(argv[i]);
    }
    assert(!argv[argc]);
    int strace_argc = argc - 1;
    char **strace_argv = NULL;
    if (strace_argc <= 0) { // 什么东西都不传给strace就报错
        assert(0);
    } else {
        strace_argc += 3; // strace -T -ttt
        // 注意strace还要加一些别的参数，整理一下先
        strace_argv = (char **)malloc(sizeof(char *) * (strace_argc + 1));
        strace_argv[0] = strdup("strace");
        strace_argv[1] = strdup("-T");
        strace_argv[2] = strdup("-ttt");
        for (int i = 1; i < argc; i++) { // 从3开始，复制剩下的参数
            strace_argv[i + 2] = strdup(argv[i]);
        }
        strace_argv[strace_argc] = NULL; // 这是最后一个参数，以NULL结尾
    }
    /*-------fork and pipe-------*/
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(1);
    }
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(1);
    }
    if (pid == 0) {
        close(pipefd[0]); // close read end
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
        dup2(pipefd[1], STDERR_FILENO); // redirect stdout to pipe
        execve("/usr/bin/strace", strace_argv, envp); // 成功传参
    } else { // parent
        close(pipefd[1]); // close write end
        FILE *fp = fdopen(pipefd[0], "r");

        // 编译正则表达式
        regex_t reg;
        const char *pattern = "([0-9]+\\.[0-9]+) ([a-zA-Z0-9_]+)\\(.*\\) = .* <([0-9]+\\.[0-9]+)>";
        if (regcomp(&reg, pattern, REG_EXTENDED) != 0) {
            perror("regcomp");
            exit(1);
        }

        syscall_info_t syscalls[MAX_SYSCALL];
        int syscall_count = 0;

        char line[4096];
        while (fgets(line, sizeof(line), fp) != NULL) {
            regmatch_t pmatch[4];
            if (regexec(&reg, line, 4, pmatch, 0) == 0) {
                // 好的，这里正在进行统计
                double start_time = atof(line + pmatch[1].rm_so);
                char name[64];
                strncpy(name, line + pmatch[2].rm_so, pmatch[2].rm_eo - pmatch[2].rm_so);
                name[pmatch[2].rm_eo - pmatch[2].rm_so] = '\0';
                double duration = atof(line + pmatch[3].rm_so);
                // 保存到syscalls数组中
                syscall_info_t *info = &syscalls[syscall_count++];
                info->start_time = start_time;
                strncpy(info->name, name, sizeof(info->name));
                info->duration = duration;
            }
        }
        double duration = syscalls[syscall_count - 1].start_time - syscalls[0].start_time;
        double total_time = 0;
        syscall_summary_t summaries[MAX_SYSCALL];
        int summary_count = 0;
        for (int i = 0; i < syscall_count; i++) {
            int found = 0;
            for (int j = 0; j < summary_count; j++) {
                if (strcmp(summaries[j].name, syscalls[i].name) == 0) {
                    summaries[j].total_time += syscalls[i].duration;
                    summaries[j].count++;
                    found = 1;
                    break; //强行加了，这里得插个flag
                }
            }
            if (found) continue;
            assert(summary_count < MAX_SYSCALL);
            strcpy(summaries[summary_count].name, syscalls[i].name);
            summaries[summary_count].total_time = syscalls[i].duration;
            summaries[summary_count].count = 1;
            summary_count++;
        }
        debug("----------------------------\n");
        qsort(summaries, summary_count, sizeof(syscall_summary_t), cmp);
        int min = summary_count > 5 ? 5 : summary_count;
        for (int i = 0; i < min; i++) {
            int ratio = summaries[i].total_time / duration * 100;
            printf("%s (%d%%)\n", summaries[i].name, ratio);
            fflush(stdout);
        }
    }


    return 0;
}
