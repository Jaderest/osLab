// DO NOT MODIFY: Will be reverted by the Online Judge.

#include <kernel.h>
#include <klib.h>
#include <os.h>

void alignTest() {
    for (const char *s = "Hello World from CPU #*\n"; *s; s++) {
        putch(*s == '*' ? '0' + cpu_current() : *s);
    }

    for (int i = 0; i < 1000; ++i) {
        void *addr = pmm->alloc(4096);
        printf("addr = %x\n", addr);
    }

    while (1) ;
}

// sem_t empty, fill;
// static void producer(void *arg) {
//     while (1) {
//         kmt->sem_wait(&empty);
//         printf("(");
//         kmt->sem_signal(&fill);
//     }
// }
// static void consumer(void *arg) {
//     while (1) {
//         kmt->sem_wait(&fill);
//         printf(")");
//         kmt->sem_signal(&empty);
//     }
// }
// static void create_threads() {
//     kmt->sem_init(&empty, "empty", 10);
//     kmt->sem_init(&fill, "fill", 0);
//     for (int i = 0; i < 10; ++i) {
//         kmt->create(pmm->alloc(sizeof(task_t)), "producer", producer, NULL);
//         kmt->create(pmm->alloc(sizeof(task_t)), "consumer", consumer, NULL);
//     }
// }


sem_t empty, fill;
static void testPrintL() {
    while (1)
    {
        kmt->sem_wait(&empty);
        putch('(');
        kmt->sem_signal(&fill);
        // log("(");
    }
}
static void testPrintR() {
    while (1)
    {
        kmt->sem_wait(&fill);
        putch(')');
        kmt->sem_signal(&empty);
        // log(")");
    }
}
static void create_threads() {
    TRACE_ENTRY;
    for (int i = 0; i < 16; ++i) {
        kmt->create(pmm->alloc(sizeof(task_t)), "producer", testPrintL, NULL);
        kmt->create(pmm->alloc(sizeof(task_t)), "consumer", testPrintR, NULL);
    }
    TRACE_EXIT;
}


int main() {
    ioe_init();
    cte_init(os->trap); // 对应thread-os的cte_init(on_interrupt);
    os->init();
    kmt->sem_init(&empty, "empty", 10);
    kmt->sem_init(&fill, "fill", 0);
    log("Hello, OS World!\n");
    create_threads();
    log("create threads\n");

    // 所有处理器运行同一份代码，拥有独立的堆栈，共享的内存
    mpe_init(os->run); // 让每个处理器都运行os->run，此时操作系统真正化身成了中断处理程序
    // mpe_init(alignTest); 
    return 1;
}
