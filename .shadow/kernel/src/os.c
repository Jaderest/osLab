#include <os.h>

#ifdef TEST
#include <am.h>
#include <stdio.h>
void putch(char ch) {
    putchar(ch);
}
#endif

//-----------------handler-----------------
typedef struct Handler {
    int seq;
    int event;
    handler_t handler;
    struct Handler *next;
    struct Handler *prev;
} Handler;
Handler *handler_head = NULL;
void handler_add(int seq, int event, handler_t handler) {
    Handler *h = pmm->alloc(sizeof(Handler));
    PANIC_ON(h == NULL, "Failed to allocate memory for handler");
    h->seq = seq;
    h->event = event;
    h->handler = handler;
    h->next = h->prev = NULL;

    if (handler_head == NULL) {
        handler_head = h;
        return;
    }
    Handler *p = handler_head;
    Handler *prev = NULL;
    while (p && p->seq < seq) {
        prev = p;
        p = p->next;
    }
    h->next = p;
    h->prev = prev;
    if (prev) prev->next = h;
    if (p) p->prev = h;
}
void print_handler() {
    Handler *p = handler_head;
    while (p) {
        printf("seq: %d, event: %d\n", p->seq, p->event);
        p = p->next;
    }
}

static void os_on_irq(int seq, int event, handler_t handler) {
    handler_add(seq, event, handler);
}

static void os_init() {
    NO_INTR;
    pmm->init();
    kmt->init();
    os_on_irq(0, 0, NULL);
    os_on_irq(3, 0, NULL);
    os_on_irq(2, 0, NULL);
    os_on_irq(1, 0, NULL);
    os_on_irq(123, 0, NULL);
    os_on_irq(2, 0, NULL);
    print_handler();

    // dev->init();
}

#ifndef TEST
static void os_run() {
    for (const char *s = "Hello World from CPU #*\n"; *s; s++) {
        putch(*s == '*' ? '0' + cpu_current() : *s);
    }
    // 以下为正确代码，但是开始神秘重启
    // TODO：研究os->trap()
    iset(true);
    // yield(); // 开始return NULL
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
    NO_INTR;
    Handler *p = handler_head;
    Context *next = NULL;
    while (p) {
        if (p->event == ev.event || p->event == EVENT_NULL) {
            Context *ret = p->handler(ev, context);
            PANIC_ON(ret && next, "returning multiple times");
            if (ret) next = ret;
        }
        p = p->next;
    }
    PANIC_ON(next == NULL, "No handler found for event %d", ev.event);
    return next;
}

// TODO2: 增加代码可维护性
/*
防止在增加新功能都去修改os trap
增加了这个中断处理api，调用这个向操作系统内核注册一个中断处理程序
在os trap执行时，当 ev.event（事件编号）和 event 匹配时，调用handler(event,ctx)
*/
/*
typedef struct {
  enum {
    EVENT_NULL = 0,
    EVENT_YIELD, EVENT_SYSCALL, EVENT_PAGEFAULT, EVENT_ERROR,
    EVENT_IRQ_TIMER, EVENT_IRQ_IODEV,
  } event;
  uintptr_t cause, ref;
  const char *msg;
} Event;
*/

MODULE_DEF(os) = {
    .init = os_init,
    .run  = os_run,
    .trap = os_trap,
    .on_irq = os_on_irq,
};
