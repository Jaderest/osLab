// DO NOT MODIFY: Will be reverted by the Online Judge.

#include <kernel.h>
#include <klib.h>
#include <os.h>

void alignTest1() {
    for (const char *s = "Hello World from CPU #*\n"; *s; s++) {
        putch(*s == '*' ? '0' + cpu_current() : *s);
    }

    while (1) ;
}

void alignTest2() {
    for (const char *s = "Hello World from CPU #*\n"; *s; s++) {
        putch(*s == '*' ? '0' + cpu_current() : *s);
    }

    while (1) ;

}

static spinlock_t lk1, lk2, lk3, lk4;

int main() {
    ioe_init();
    cte_init(os->trap);
    os->init();
    mpe_init(os->run); // 让每个处理器都运行os->run，此时操作系统真正化身成了中断处理程序
    printf("lock addr: %x\n", &lk1);
    printf("lock addr: %x\n", &lk2);
    printf("lock addr: %x\n", &lk3);
    printf("lock addr: %x\n", &lk4);
    // mpe_init(alignTest1);
    // mpe_init(alignTest2);
    return 1;
}
