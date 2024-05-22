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
    // debug("path = %s\n", path);
    /*Fork: Creating parent and child*/
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    if (pid == 0) { // Child
        close(pipefd[0]); // Close read end
        close(STDERR_FILENO); // 关闭不必要的输出
        close(STDOUT_FILENO);

        // 执行strace，并不断输出到pipefd[1]
        // -o pipefd[1]表示输出到pipefd[1]，argv[1]是要执行的程序
        char *exec_argc[] = {"strace", "-T", "-ttt", "-o", "pipefd[1]", argv[1], NULL}; //的确是丢到管道里了，一会在父进程中检测一下是否有输出
        char *exec_envp[] = {"PATH=/usr/bin", NULL};
        debug("execve\n");
        execve("/usr/bin/strace", exec_argc, exec_envp); // 没找到strace，所以这里会报错
        perror("execve");
        //TODO: 我要想想怎么执行这个command，然后参数该怎么样处理，然后搜索环境变量的方式要了解一下，参考jyy给的手动模拟

        exit(EXIT_SUCCESS);
    }

    // char *exec_argv[] = {"strace", "-T", "-ttt", "ls", NULL}; // 这里把ls换成argv[1]（若它是以/开头的绝对路径，则直接执行，否则在PATH中搜索），然后argv[2]是argv[1]的参数
    // char *exec_envp[] = {"PATH=", NULL};
    // // char *exec_envp[] = {"", NULL};
    // execve("strace", exec_argv, exec_envp);
    // execve("/bin/strace", exec_argv, exec_envp);
    // execve("/usr/bin/strace", exec_argv, exec_envp);
    // perror(argv[0]);
    // exit(EXIT_FAILURE);

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
