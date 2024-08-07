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

void testL() {
  while (1) {
    printf("(");
  }
}
void testR() {
  while (1) {
    printf(")");
  }
}

sem_t empty, fill;
void producer(void *arg) {
  while (1) {
    kmt->sem_wait(&empty);
    putch('(');
    kmt->sem_signal(&fill);
  }
}
void consumer(void *arg) { // 这个就是先获取fill
  while (1) {
    // log("before wait\n");
    kmt->sem_wait(
        &fill); // 那么这个线程当前就应该阻塞在这个位置，然后需要让出cpu，运行其他线程，直至信号量解封它
    // 但是事实是这个cpu发生了一次中断并进入调度，然后重新选了这个线程，然后发生奇怪的死锁
    /*
    current->name:consumer to cpu 0
    here
    task unlock
    before wait
    [TRACE in 0] /home/jaderest/os-workbench/kernel/src/kmt.c: kmt_sem_wait:
    243: Entry sem->name:fill if cpu 0: 2 times schedule not idle
    current->name:consumer to cpu 0
    here
    task unlock
    cpu 0: 3 times schedule
    not idle
    current->name:producer to cpu 0
    here
    task unlock
    [TRACE in 0] /home/jaderest/os-workbench/kernel/src/kmt.c: kmt_sem_wait:
    243: Entry sem->name:empty else sem unlock [1;41mPanic:
    /home/jaderest/os-workbench/kernel/src/kmt.c:264: Interrupt is disabled[0m
    比如以上输出，首先cpu0的第一次中断，将fill那个线程切换上来，没有解锁（很奇怪这里为什么会立马中断并且没有解锁），然后立马跳到另一个empty的线程，然后获取锁的时候发现获取的是同一把锁，明明是两个线程啊？
    所以一会就要检查中间为什么会立即发生一次中断
    */
    putch(')');
    kmt->sem_signal(&empty);
  }
}

task_t *task_alloc() { return pmm->alloc(sizeof(task_t)); }

// static void run_test1() {
//   kmt->sem_init(&empty, "empty", 3);
//   kmt->sem_init(&fill, "fill", 0);
//   for (int i = 0; i < 1; i++) {
//     kmt->create(task_alloc(), "producer", producer, NULL);
//   }
//   for (int i = 0; i < 1; i++) {
//     kmt->create(task_alloc(), "consumer", consumer, NULL);
//   }
// }

static void os_init() {
  NO_INTR;
  pmm->init();
  kmt->init();
  printf("init done\n");
  for (int i = 0; i < 2; i++) {
    kmt_create(task_alloc(), "testL", testL, NULL);
    kmt_create(task_alloc(), "testR", testR, NULL);
  }
  // run_test1();
  // dev->init();
  print_handler(); // 为什么你可以用log
  NO_INTR;
}

#ifndef TEST
static void os_run() {
  // for (const char *s = "Hello World from CPU #*\n"; *s; s++) {
  //   putch(*s == '*' ? '0' + cpu_current() : *s);
  // }
  // 给当前cpu开中断之后就会立刻运行task，然后就会出问题
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
  // TRACE_ENTRY;
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
  // 保存了一下当前的text
  NO_INTR;
  // TRACE_EXIT;
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
