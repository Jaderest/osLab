#include <devices.h>
#include <os.h>

#ifdef TEST
#include <am.h>
#include <stdio.h>
void putch(char ch) { putchar(ch); }
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
  if (prev)
    prev->next = h;
  if (p)
    p->prev = h;
}
void print_handler() {
  Handler *p = handler_head;
  while (p) {
    log("seq: %d, event: %d\n", p->seq, p->event);
    p = p->next;
  }
}

static void os_on_irq(int seq, int event, handler_t handler) {
  handler_add(seq, event, handler);
}

task_t *task_alloc() { return pmm->alloc(sizeof(task_t)); }

static void os_init() {
  NO_INTR;
  pmm->init();
  kmt->init();
  // dev->init();
  print_handler(); // 为什么你可以用log
  NO_INTR;
}

#ifndef TEST
static void os_run() {
  iset(true);
  yield();

  // 观察课程群大佬的issue发现这个yield其实也不必要
  while (1)
    ;
}
#else
static void os_run() {}
#endif

/*
中断/异常发生后，am会将寄存器保存到栈上，建议对context做一个拷贝，并实现上下文切换
每个处理器都各自管理中断，使用自旋锁保护 //! 共享变量
*/
static Context *os_trap(Event ev, Context *context) {
  NO_INTR; // 确保中断是关闭的，中断确实是关的，但是task是有可能数据竞争的对吧
  
  Handler *p = handler_head;
  Context *next = NULL;
  int irq_num = 0;
  while (p) {
    if (p->event == ev.event || p->event == EVENT_NULL) {
      Context *ret = p->handler(ev, context);
      PANIC_ON(ret && next, "returning multiple times");
      if (ret)
        next = ret;
    }
    irq_num++;
    p = p->next;
  }

  NO_INTR;
  PANIC_ON(next == NULL, "No handler found for event %d", ev.event);
  return next;
}
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
    .run = os_run,
    .trap = os_trap,
    .on_irq = os_on_irq,
};
