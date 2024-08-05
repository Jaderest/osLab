#include <os.h>

static int total_nt = 0;              // 任务总数
static task_t idle[MAX_CPU_NUM] = {}; // 每个cpu的idle task（空闲任务）
static task_t *tasks[MAX_THREAD] = {};     // 所有的task
static task_t *currents[MAX_CPU_NUM] = {}; // 每个cpu当前运行的task
#define current currents[cpu_current()]

static spinlock_t task_lock = spinlock_init("task_lock");

void init_stack_guard(task_t *task) {
  for (int i = 0; i < STACK_GUARD_SIZE; i++) {
    task->stack_fense_s[i] = STACK_GUARD_VALUE;
    task->stack_fense_e[i] = STACK_GUARD_VALUE;
  }
}

int check_stack_guard(task_t *task) {
  for (int i = 0; i < STACK_GUARD_SIZE; i++) {
    if (task->stack_fense_s[i] != STACK_GUARD_VALUE ||
        task->stack_fense_e[i] != STACK_GUARD_VALUE) {
      return 0;
    }
  }
  return 1;
}

int _create(task_t *task, const char *name, void (*entry)(void *arg),
            void *arg) {
  _spin_lock(&task_lock); // 一把task的大锁
  NO_INTR;

  task->name = name;
  task->status = RUNNABLE;
  init_stack_guard(task);

  Area stack = (Area){task->stack, task->stack + STACK_SIZE};
  task->context = kcontext(stack, entry, arg);
  task->id = total_nt;
  tasks[total_nt] = task;
  total_nt++;
  PANIC_ON(!stack_check(task), "Stack overflow");

  NO_INTR;
  _spin_unlock(&task_lock);

  return 0;
}

void _teardown(task_t *task) {
    PANIC_ON(task->status != ZOMBIE && task->status , "Cannot teardown a running task");
    _spin_lock(&task_lock);
    tasks[task->id] = NULL;
    pmm->free(task);
    _spin_unlock(&task_lock);
}

void idle_init() {
  for (int i = 0; i < cpu_count(); i++) {
    currents[i] = &idle[i];
    idle[i].status = RUNNING;
    idle[i].id = 0;
    idle[i].name = "idle";
    init_stack_guard(&idle[i]);
  }
}

Context *kmt_context_save(Event ev, Context *ctx) {
  TRACE_ENTRY;
  NO_INTR;
  PANIC_ON(!check_stack_guard(current), "Stack overflow detected in CPU #%d\n",
           cpu_current());

  if (current->status != BLOCKED)
    current->status = RUNNABLE;
  current->context = ctx;

  NO_INTR;
  TRACE_EXIT;
  return NULL;
}

// thread starvation 那应该调整我的调度策略
Context *kmt_schedule(Event ev, Context *ctx) {
  TRACE_ENTRY;
  NO_INTR;

  int index = current->id;
  int i = 0;
  while (i < total_nt * 10) {
    index = (index + 1) % total_nt;
    if (tasks[index]->status == RUNNABLE)
      break;
    i++;
  }
  if (i == 10 * total_nt) {
    current = &idle[cpu_current()]; // 找不到可以运行的
  } else {
    current = tasks[index];
  }
  current->status = RUNNING;

  NO_INTR;
  PANIC_ON(!check_stack_guard(current), "Stack overflow detected in CPU #%d\n",
           cpu_current());
  TRACE_EXIT;
  return current->context;
}

// 必定保护在信号量的锁里面，这个接口不写在外面
void _sem_queue_push(task_queue_t *queue, task_t *task) {
  task_node_t *node = pmm->alloc(sizeof(task_node_t));
  PANIC_ON(!node, "Failed to allocate memory for task node");
  node->task = task;
  node->prev = queue->tail;
  node->next = NULL;
  if (queue->tail) {
    queue->tail->next = node;
  } else {
    queue->head = node;
  }
  queue->tail = node;
}
task_t *_sem_queue_pop(task_queue_t *queue) {
  if (!queue->head) {
    PANIC("Semaphore queue is empty");
    return NULL;
  }
  task_node_t *node = queue->head;
  task_t *task = node->task;

  queue->head = node->next;
  if (queue->head) {
    queue->head->prev = NULL;
  } else {
    queue->tail = NULL;
  }

  pmm->free(node);
  return task;
}

void _sem_init(sem_t *sem, const char *name, int value) {
  sem->name = name;
  sem->value = value;
  sem->queue = NULL;
  sem->lk = spinlock_init(name);
}

// P 操作（取球）
/*
允许在线程中执行信号量的
P，但是P操作没有相应资源时，线程将被阻塞（不再被调度执行，？这里是不再还是可以恢复成其他状态）
中断没有对应的线程，不能阻塞，因此不能在中断时调用 P
*/
void _sem_wait(sem_t *sem) {
  int flag = 0;         // 标志P操作是否失败
  _spin_lock(&sem->lk); // 即使用锁阻塞在这里
  sem->value--;
  if (sem->value < 0) {
    flag = 1;
    current->status = BLOCKED;
    _sem_queue_push(sem->queue, current);
  }
  _spin_unlock(&sem->lk);
  if (flag) { // 若 P 操作失败，则不能继续执行
    // 有可能会有线程执行 V 操作
    INTR;
    yield();
  }
}

// V 操作（放球）
/*
允许任意状态下执行V，包括任何处理器中的任何线程，任何处理器的任何中断
取出当前队列的一个元素，给它标记为可运行（标记为可运行那就会运行上去）
然后os-trap会保证存储这个线程运行在cpu上的状态
*/
void _sem_signal(sem_t *sem) {
  _spin_lock(&sem->lk); // 锁住了当前信号量的，然后关中断了
  NO_INTR;
  sem->value++;
  if (sem->value <= 0) { // 说明原先的信号量是小于0的？等下这是什么意思
    PANIC_ON(!sem->queue, "Semaphore queue is empty");
    task_t *task = _sem_queue_pop(sem->queue);
    task->status = RUNNABLE;
  }
  NO_INTR;
  _spin_unlock(&sem->lk);
}