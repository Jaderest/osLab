// DO NOT MODIFY: Will be reverted by the Online Judge.

#include <kernel.h>
#include <klib.h>
#include <os.h>

#ifdef LOG
spinlock_t log_lk = spinlock_init("log");
#endif

void alignTest1() {
    for (const char *s = "Hello World from CPU #*\n"; *s; s++) {
        putch(*s == '*' ? '0' + cpu_current() : *s);
    }

    for (int i = 0; i < 1000; ++i) {
        void *addr = pmm->alloc(4096);
        log("addr = %x\n", addr);
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
    //TODO1：所以我要实现os->trap()
    cte_init(os->trap); // 对应thread-os的cte_init(on_interrupt);
    os->init();

    // mpe_init(os->run); // 让每个处理器都运行os->run，此时操作系统真正化身成了中断处理程序
    mpe_init(alignTest1); 
    printf("lock addr: %x\n", &lk1);
    printf("lock addr: %x\n", &lk2);
    printf("lock addr: %x\n", &lk3);
    printf("lock addr: %x\n", &lk4);
    // mpe_init(alignTest1);
    // mpe_init(alignTest2);
    return 1;
}
