// DO NOT MODIFY: Will be reverted by the Online Judge.

#include <kernel.h>
#include <klib.h>
#include <os.h>

#ifdef LOG
spinlock_t log_lk = spinlock_init("log");
#endif

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
//     kmt->create(pmm->alloc(sizeof(task_t)), "producer", producer, NULL);
//     kmt->create(pmm->alloc(sizeof(task_t)), "consumer", consumer, NULL);
// }


int main() {
    printf("before pmm\n");
    ioe_init();
    printf("before os\n");
    cte_init(os->trap); // 对应thread-os的cte_init(on_interrupt);
    printf("before os_init\n");
    os->init();
    printf("init done\n");
    // create_threads();

    // 所有处理器运行同一份代码，拥有独立的堆栈，共享的内存
    mpe_init(os->run); // 让每个处理器都运行os->run，此时操作系统真正化身成了中断处理程序
    // mpe_init(alignTest); 
    return 1;
}
