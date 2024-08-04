#include <os.h>

#ifdef TEST
#include <am.h>
#include <stdio.h>
void putch(char ch) {
    putchar(ch);
}
#endif

#define N 10
// #define DEBUG_LOCAL
#ifdef DEBUG_LOCAL
static void run_test1() {
    static sem_t empty, fill;
    kmt->sem_init(&empty, "empty", N);
    kmt->sem_init(&fill,  "fill",  0);
    log("empty addr = %x\n", &empty);
    log("fill addr = %x\n", &fill);
}
#endif

static void os_init() {
    pmm->init();
    kmt->init();
    
#ifdef DEBUG_LOCAL
    run_test1();
#endif
    // dev->init();
}

#ifndef TEST
static void os_run() {
    for (const char *s = "Hello World from CPU #*\n"; *s; s++) {
        putch(*s == '*' ? '0' + cpu_current() : *s);
    }
    while (1) ;
}
#else
static void os_run() {
    
}
#endif

//TODO1: os->trap()的实现
/*
中断/异常发生后，am会将寄存器保存到栈上，建议对context做一个拷贝，并实现上下文切换
每个处理器都各自管理中断，使用自旋锁保护 //! 共享变量
*/
static Context *os_trap(Event ev, Context *context) {
    return NULL;
}

// TODO2: 增加代码可维护性
/*
防止在增加新功能都去修改os trap
增加了这个中断处理api，调用这个向操作系统内核注册一个中断处理程序
在os trap执行时，当 ev.event（事件编号）和 event 匹配时，调用handler(event,ctx)
*/
static void os_on_irq(int seq, int event, handler_t handler) {
    
}

MODULE_DEF(os) = {
    .init = os_init,
    .run  = os_run,
    .trap = os_trap,
    .on_irq = os_on_irq,
};
