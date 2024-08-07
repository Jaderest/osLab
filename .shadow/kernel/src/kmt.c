#include <klib-macros.h>
#include <klib.h>
#include <os.h>
#include <stdint.h>

struct cpu cpus[MAX_CPU_NUM];

#ifdef LOG
spinlock_t log_lk = spinlock_init("log");
#endif

#define MAX_TASK_NUM 128              // 最多支持128个任务
static task_t idle[MAX_CPU_NUM];      // cpu 上空转的任务

static mutexlock_t task_lk; // 在kmt->init()
static spinlock_t task_lk_spin = spinlock_init("task_lk");
//--------protected in task_lk---------
static task_t *tasks[MAX_TASK_NUM]; // all tasks
static int total_task_num = 0;
static task_t *currents[MAX_CPU_NUM]; // 当前任务
#define current currents[cpu_current()]
//-------------------------------------

#define stack_check(task)                                                      \
  PANIC_ON(check_stack_guard(task), "%s stack overflow in %d", (task)->name,   \
           cpu_current())

//------------------spinlock------------------
void kmt_spin_init(spinlock_t *lk, const char *name) { _spin_init(lk, name); }
void kmt_spin_lock(spinlock_t *lk) { _spin_lock(lk); }
void kmt_spin_unlock(spinlock_t *lk) { _spin_unlock(lk); }
//-----------------E-spinlock------------------

//------------mutexlock-------------
void queue_init(task_queue_t *queue) {
  queue = pmm->alloc(sizeof(task_queue_t));
  queue->head = NULL;
  queue->tail = NULL;
}
int queue_empty(task_queue_t *queue) {
  return (queue->head == NULL && queue->tail == NULL);
}
void queue_push(task_queue_t *queue, task_t *task) {
  task_node_t *node = pmm->alloc(sizeof(task_node_t));
  PANIC_ON(node == NULL, "node alloc err");

  node->task = task;
  node->prev = queue->tail;
  node->next = NULL;
  if (queue->tail != NULL) { // 非空队列
    queue->tail->next = node;
  } else { // 空队列
    queue->head = node;
  }
  queue->tail = node;
}
task_t *queue_pop(task_queue_t *queue) {
  if (queue->head == NULL)
    return NULL;
  task_node_t *node = queue->head;
  task_t *task = node->task;

  queue->head = node->next;
  if (queue->head != NULL) {
    queue->head->prev = NULL;
  } else { // queue->head == NULL
    queue->tail = NULL;
  }
  pmm->free(node);
  return task;
}
void mutex_init(mutexlock_t *lk, const char *name) {
  lk->locked = 0;
  queue_init(lk->wait_list);
  _spin_init(&lk->spinlock, name);
}
void mutex_lock(mutexlock_t *lk) {
  int acquired = 0;
  _spin_lock(&lk->spinlock);
  if (lk->locked != 0) {
    queue_push(lk->wait_list, current);
    current->status = BLOCKED;
  } else {
    lk->locked = 1;
    acquired = 1;
  }
  _spin_unlock(&lk->spinlock);
  if (!acquired)
    yield(); // 主动切换到其他线程执行
}
void mutex_unlock(mutexlock_t *lk) {
  _spin_lock(&lk->spinlock);
  if (!queue_empty(lk->wait_list)) {
    task_t *task = queue_pop(lk->wait_list);
    task->status = RUNNABLE; // 唤醒之前睡眠的线程
  } else {
    lk->locked = 0;
  }
  _spin_unlock(&lk->spinlock);
}
//----------E-mutexlock-----------

Context *kmt_context_save(Event ev, Context *ctx) {
  NO_INTR; // 确保中断是关闭的，这里是不是am主动关上的中断，中断处理函数就必须关中断
  stack_check(current);

  current->context = ctx; //保存当前的context

  NO_INTR;
  return NULL;
}

Context *kmt_schedule(Event ev, Context *ctx) {
  // 获取可以运行的任务
  // int index = current->id;

  // log("---------monitor---------\n");
  // for (int i = 0; i < cpu_count(); ++i) {
  //   log("cpu%d: %s\n", i, currents[i]->name);
  // }
  // for (int i = 0; i < total_task_num; ++i) {
  //   log("task%d: %s\n", i, tasks[i]->name);
  // }
  // log("--------E-monitor---------\n");

  int index = rand() % total_task_num;
  int i = 0;
  NO_INTR;

  // mutex_lock(&task_lk);
  for (i = 0; i < total_task_num * 10; ++i) {
    index = (index + 1) % total_task_num;
    if (tasks[index]->status == RUNNABLE) {
      break;
    } else if (tasks[index] == NULL) {
      continue;
    } else if (tasks[index]->status == RUNNING || tasks[index]->status == BLOCKED) {
      continue;
    } else { // DEAD
      tasks[index] = NULL;
    }
  }

  stack_check(current);
  if (i == total_task_num * 10) {
    current->status = RUNNABLE; // 作为前一个线程，重新加入可运行队列

    log("no task to run, idle\n");
    current = &idle[cpu_current()];
    current->status = RUNNING;
  } else {
    log("current task: %s -> ", current->name);
    current->status = RUNNABLE;
    current = tasks[index];
    log("next task: %s\n", current->name);
    current->status = RUNNING;
  }
  stack_check(current);
  // mutex_unlock(&task_lk);

  NO_INTR;
  return current->context;
  return NULL;
}

void init_stack_guard(task_t *task) {
  for (int i = 0; i < STACK_GUARD_SIZE; ++i) {
    task->stack_fense_s[i] = STACK_GUARD_VALUE;
    task->stack_fense_e[i] = STACK_GUARD_VALUE;
  }
}
int check_stack_guard(task_t *task) {
  for (int i = 0; i < STACK_GUARD_SIZE; ++i) {
    if (task->stack_fense_s[i] != STACK_GUARD_VALUE ||
        task->stack_fense_e[i] != STACK_GUARD_VALUE) {
      return 1;
    }
  }
  return 0;
}

void task_init(task_t *task, const char *name) {
  task->name = name;
  task->status = RUNNABLE;
  init_stack_guard(task);
}

void idle_init() {
  for (int i = 0; i < cpu_count(); ++i) { // 先初始化在每个cpu上
    currents[i] = &idle[i];
    idle[i].cpu_id = i;
    idle[i].status = RUNNING; // 空转线程，开机的时候转在cpu上的
    currents[i]->name = "idle";
    init_stack_guard(&idle[i]);
    stack_check(&idle[i]);
  }
}

void kmt_init() {
  os->on_irq(INT_MIN, EVENT_NULL, kmt_context_save);
  os->on_irq(INT_MAX, EVENT_NULL, kmt_schedule);
  mutex_init(&task_lk, "task_lock");
  idle_init();
}

// task的内存已预先分配好，并且允许任何线程调用task_create
int kmt_create(task_t *task, const char *name, void (*entry)(void *arg),
               void *arg) {
  TRACE_ENTRY;

  task_init(task, name);
  Area stack = (Area){task->stack, task->stack + STACK_SIZE};

  task->context = kcontext(stack, entry, arg);
  init_stack_guard(task);

  log("task %s created\n", name);
  _spin_lock(&task_lk_spin); // 保护全局变量
  // 使用互斥锁的话这里是没有中断的，但是保护了task_lk需要保护的东西

  task->id = total_task_num;
  tasks[total_task_num] = task;
  total_task_num++;

  _spin_unlock(&task_lk_spin);
  log("unlock\n");

  stack_check(current);
  TRACE_EXIT;
  return 0;
}

// 这个是不是也要注册成处理函数，不需要，写在schedule就行
void kmt_teardown(task_t *task) {
  // TODO：
}

//------------------sem------------------
void kmt_sem_init(sem_t *sem, const char *name, int value) {
  TRACE_ENTRY;
  sem->name = name;
  sem->value = value;
  char dst[256] = "";
  snprintf(dst, strlen(name) + 4 + 1, "sem-%s", name);
  mutex_init(&sem->lk, dst);
  queue_init(sem->queue);
  TRACE_EXIT;
}

// 怎么这里进了两次这个函数？观察一下cpu
// 都是cpu0上的，cnm我现在只启动了一个cpu，肯定是0
void kmt_sem_wait(sem_t *sem) {
  TRACE_ENTRY;
  INTR;


  INTR;
  TRACE_EXIT;
}

void kmt_sem_signal(sem_t *sem) {
  TRACE_ENTRY;
  INTR;

  INTR;
  TRACE_EXIT;
}
//------------------sem------------------

MODULE_DEF(kmt) = {
    .init = kmt_init,
    .create = kmt_create,
    .teardown = kmt_teardown,
    .spin_init = kmt_spin_init,
    .spin_lock = kmt_spin_lock,
    .spin_unlock = kmt_spin_unlock,
    .sem_init = kmt_sem_init,
    .sem_wait = kmt_sem_wait,
    .sem_signal = kmt_sem_signal,
};