#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>
#include <unistd.h>
#include <fcntl.h>

#define DEBUG
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
} syscall_info_t; // 这个数据结构肯定不行捏，必爆内存

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
        // assert(0);
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
    for (int i = 0; i < strace_argc; i++) {
        debug("strace_argv[%d] = %s\n", i, strace_argv[i]);
        fflush(stdout);
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
        close(STDERR_FILENO);
        // 事实上这个dup2和>比较像，但是这个是系统调用，而>是shell的功能，都是重定向
        // dup2(pipefd[1], STDERR_FILENO); // redirect stdout to pipe
        debug("execve\n");
        // int fd = open("/dev/null", O_WRONLY); //这样stdout就不会输出了
        // dup2(fd, STDOUT_FILENO);
        //TODO：以上de好环境问题再解除注释
        /**
         * filename：是相对于进程的当前目录
        */
        //TODO: envp传参
        execve("/usr/bin/strace", strace_argv, envp); // 成功传参
        for (int i = 0; strace_argv[i] != NULL; i++) {
            debug("strace_argv[%d] = %s\n", i, strace_argv[i]);
        }
        debug("execve\n");
        int fdtty = open("/dev/tty", O_WRONLY);
        dup2(fdtty, STDERR_FILENO);
        perror("execve"); // 果然传yes会出现问题
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

        char line[4096];
        while (fgets(line, sizeof(line), fp) != NULL) {
            // debug("%s", line);
            regmatch_t pmatch[4];
            if (regexec(&reg, line, 4, pmatch, 0) == 0) {
                // 好的，这里正在进行统计
                double start_time = atof(line + pmatch[1].rm_so);
                char name[64];
                strncpy(name, line + pmatch[2].rm_so, pmatch[2].rm_eo - pmatch[2].rm_so);
                name[pmatch[2].rm_eo - pmatch[2].rm_so] = '\0';
                double duration = atof(line + pmatch[3].rm_so);
                // 以上是匹配结果，接下来该怎么做

            }
        }
        double duration = 0;
        double total_time = 0;
        
    }


    return 0;
}
