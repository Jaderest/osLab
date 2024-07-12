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
        //TODO 子进程
        close(pipefd[0]); // close read end
        dup2(pipefd[1], STDERR_FILENO); // redirect stderr to pipe，即将strace的输出重定向过去
        int fd = open("/dev/null", O_WRONLY);
        dup2(fd, STDOUT_FILENO); // 不输出子进程的其他东西
        // TODO：准备一下argv

        debug("execve(\"/usr/bin/strace\", argv, envp);\n");
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
        //  = {
        //     "/usr/bin/strace",
        //     "-T", "-ttt", //TODO: 底下拷进去那个argv里面的东西
        //     "yes",
        //     NULL,
        // };
        execve("/usr/bin/strace", strace_argv, envp);
        // 执行了上面的发现write没有输出，说明execve是把当前进程变成了strace，之后的代码不会执行
        // write(pipefd[1], "hello", 5);
    } else if (pid > 0) {
        //TODO 父进程
        close(pipefd[1]); // close write end
        char buffer[4096];
        ssize_t bytes_read;
        while ((bytes_read = read(pipefd[0], buffer, sizeof(buffer) - 1)) > 0) {
            buffer[bytes_read] = '\0'; // 确保字符串以 '\0' 结尾
            debug("%s", buffer);
        }

        if (bytes_read == -1) {
            perror("read");
            return 1;
        }
    } else {
        perror("fork()");
        return 1;
    }

    return 0;
}
