#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    char *exec_argv[] = { "strace", "ls", NULL, };
    char *exec_envp[] = { "PATH=/bin", NULL, };
    printf("1\n");
    execve("strace",          exec_argv, exec_envp);
    printf("2\n");
    execve("/bin/strace",     exec_argv, exec_envp);
    printf("3\n");
    execve("/usr/bin/strace", exec_argv, exec_envp);
    perror(argv[0]);
    exit(EXIT_FAILURE);
}