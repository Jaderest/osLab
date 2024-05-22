#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>
#include <unistd.h>

#define DEBUG
#ifdef DEBUG
#define debug(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define debug(fmt, ...)
#endif

#define MAX_CALLS 1024

int main(int argc, char *argv[], char *envp[]) {
    for (int i = 0; i < argc; i++) {
        assert(argv[i]);
        debug("argv[%d] = %s\n", i, argv[i]);
    }
    assert(!argv[argc]);
    /*Init: Pipe, Environment*/
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }
    char *path = getenv("PATH"); // 环境变量
    /*Fork: Creating parent and child*/
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    // if (pid == 0) { // Child
    //     close(pipefd[0]); // Close read end
    //     close(STDERR_FILENO); // 关闭不必要的输出
    //     close(STDOUT_FILENO);

    //     // 执行strace，并不断输出到pipefd[1]
    //     dup2(pipefd[1], STDERR_FILENO);
    //     char *exec_argc[] = {"strace", "-T", "-e", "-ttt", "trace=", "-o", "pipefd[1]", argv[1], NULL};
    //     //TODO: 我要想想怎么执行这个command，然后参数该怎么样处理，然后搜索环境变量的方式要了解一下，参考jyy给的手动模拟

    //     exit(EXIT_SUCCESS);
    // }

    execve("/usr/bin/yes", argv, NULL);
    perror("execve"); //这里输出了两遍，fork导致的

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
