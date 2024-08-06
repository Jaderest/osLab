#include <os.h>
#include <stdint.h>
#include <klib.h>
#include <klib-macros.h>

struct cpu cpus[MAX_CPU_NUM];

#ifdef LOG
spinlock_t log_lk = spinlock_init("log");
#endif

#define MAX_TASK_NUM 128 // 最多支持128个任务
static task_t idle[MAX_CPU_NUM];     // cpu 上空转的任务
static task_t *currents[MAX_CPU_NUM]; // 当前任务
// static task_t *buffer[MAX_CPU_NUM]; // 当前cpu的上一个任务，或许优化一下调度策略
static task_t *tasks[MAX_TASK_NUM];  // all tasks
static int total_task_num = 0;
static spinlock_t task_lk = spinlock_init("task"); // 用宏初始化了，免得麻烦
#define current currents[cpu_current()]

#define stack_check(task) \
PANIC_ON(check_stack_guard(task), "%s stack overflow in %d", (task)->name, cpu_current())

// 保存context
// ctx传的是当前cpu的当前Context，那么idle此时是不用创建context的
// kmt_create的是现在cpu要跑的任务
Context *kmt_context_save(Event ev, Context *ctx) { // 在os->trap里面调用，那么处理的便是当前cpu的任务，可以直接current
    NO_INTR;
    stack_check(current);

    _spin_lock(&task_lk);
    current->status = RUNNABLE; // 当前任务切换为可执行，初始情况其实是设置的idle，但是idle不在task队列里面
    current->cpu_id = -1;
    // 于是在schedule时可以assert检查idle
    current->context = ctx; // 保存当前的context
    _spin_unlock(&task_lk);

    stack_check(current);
    NO_INTR;
    return NULL;
}

//idle 应该写错了
int count[MAX_CPU_NUM] = {0};
Context *kmt_schedule(Event ev, Context *ctx) { // ?理一下思路先，不急着跑代码
    // 获取可以运行的任务
    count[cpu_current()]++;
    log("cpu %d: %d times schedule\n", cpu_current(), count[cpu_current()]);
#ifdef  MONITOR
    if (cpu_current() == cpu_count() - 1) { //  单独针对这个cpu
        log("--------monitor-------\n");
        for (int i = 0; i < cpu_count(); ++i) {
            log("monitor:cpu %d: %s\n", i, currents[i]->name);
        }
        for (int i = 0; i < total_task_num; ++i) {
            log("monitor:task %d: %s status = %d in cpuid %d\n", i, tasks[i]->name, tasks[i]->status, tasks[i]->cpu_id);
        }
        current = &idle[cpu_current()];
        current->status = RUNNING;
        log("--------Umonitor-------\n");
        return current->context;
    }
#endif
    NO_INTR;
    _spin_lock(&task_lk);
    stack_check(current);

    int index = rand() % total_task_num;
    int i = 0;
    for (i = 0; i < total_task_num * 10; ++i) { // 循环十遍
        index = (index + 1) % total_task_num; // index也跟着循环
        if (tasks[index] == NULL) { //TODO: teardown? 实现完信号量再看
            continue;
        }
        if (tasks[index]->status == RUNNABLE) { // 只有runnable可以break
            current = tasks[index];
            break;
        }
    }
    NO_INTR;
    // 处理获取结果
    PANIC_ON(!holding(&task_lk), "cnm");

    //idle是不可能被阻塞的
    PANIC_ON(idle[cpu_current()].status == BLOCKED, "idle blocked!");

    if (i == total_task_num * 10) {
        PANIC_ON(idle[cpu_current()].status != RUNNABLE, "idle err in cpu %d", cpu_current());
        current = &idle[cpu_current()];
        log("idle\n");
    } else {
        current = tasks[index];
        // current->status = RUNNING; //? 我这里原来是写的RUNNABLE，牛魔的copilot
        log("not idle\n");
        /**
         * 捋一下，我是第一次调度的时候把current设置成了task，这次调度是没有问题的，此时它也是runnable
         * 然后下一步，它开始运行了，运行信号量sem_wait，然后就锁死在这里了
         * 反正就是和信号量兼容一坨四，想想怎么写呢，要不要yield
         */
    }
    log("here\n");
    current->status = RUNNING; //! 这里仍然是RUNNIG
    current->cpu_id = cpu_current();

    _spin_unlock(&task_lk);
    log("task unlock\n");
    NO_INTR;
    stack_check(current);
    return current->context;
}

void init_stack_guard(task_t *task) {
    for (int i = 0; i < STACK_GUARD_SIZE; ++i) {
        task->stack_fense_s[i] = STACK_GUARD_VALUE;
        task->stack_fense_e[i] = STACK_GUARD_VALUE;
    }
}
int check_stack_guard(task_t *task) {
    for (int i = 0; i < STACK_GUARD_SIZE; ++i) {
        if (task->stack_fense_s[i] != STACK_GUARD_VALUE || task->stack_fense_e[i] != STACK_GUARD_VALUE) {
            return 1;
        }
    }
    return 0;
}

void task_init(task_t *task, const char *name) {
    task->name = name;
    task->status = RUNNABLE;
    task->cpu_id = -1;
    task->on_sem = 0; // 表示不在sem里
}

void idle_init() {
    for (int i = 0; i < cpu_count(); ++i) { // 先初始化在每个cpu上
        currents[i] = &idle[i];
        currents[i]->status = RUNNING;
        currents[i]->name = "idle";
        // 试一下只有这个几个空转会不会出问题
        init_stack_guard(&idle[i]);

        stack_check(&idle[i]);
    }
}

void kmt_init() {
    os->on_irq(INT_MIN, EVENT_NULL, kmt_context_save);
    os->on_irq(INT_MAX, EVENT_NULL, kmt_schedule);
    idle_init(); // 开机阶段，这个时候需要上锁保护吗？
}

// task的内存已预先分配好，并且允许任何线程调用task_create
int kmt_create(task_t *task, const char *name, void (*entry)(void *arg), void *arg) {
    TRACE_ENTRY;

    task_init(task, name);
    Area stack = (Area) {task->stack, task->stack + STACK_SIZE};

    task->context = kcontext(stack, entry, arg);
    init_stack_guard(task);

    _spin_lock(&task_lk); // 保护全局变量
    NO_INTR;
    tasks[total_task_num] = task;
    total_task_num++;
    NO_INTR;
    _spin_unlock(&task_lk);

    stack_check(current);
    TRACE_EXIT;
    return 0;
}

// 这个是不是也要注册成处理函数，不需要，写在schedule就行
void kmt_teardown(task_t *task) {
    //TODO：
}

//------------------spinlock------------------
void kmt_spin_init(spinlock_t *lk, const char *name) {
    _spin_init(lk, name);
}

void kmt_spin_lock(spinlock_t *lk) {
    _spin_lock(lk);
}

void kmt_spin_unlock(spinlock_t *lk) {
    _spin_unlock(lk);
}
//------------------spinlock------------------

//------------------sem------------------
void sem_queue_push(sem_t *sem, task_t *task) {
    // _spin_lock(&sem->lk); //哎呀接口没写好，重构一下
    //! 是不是这里不需要上锁的，，它就是在锁里面工作的来着
    task_node_t *node = pmm->alloc(sizeof(task_node_t));
    PANIC_ON(node == NULL, "sem queue push err");

    atomic_xchg(&task->on_sem, 1);
    
    node->task = task;
    node->prev = sem->queue->tail;
    node->next = NULL;
    if (sem->queue->tail != NULL) {
        sem->queue->tail->next = node;
    } else {
        sem->queue->head = node;
    }
    sem->queue->tail = node;
    // _spin_unlock(&sem->lk);
}
task_t *sem_queue_pop(sem_t *sem) {
    if (sem->queue->head == NULL) return NULL;
    task_node_t *node = sem->queue->head;
    task_t *task = node->task;
    atomic_xchg(&task->on_sem, 0);

    sem->queue->head = node->next;
    if (sem->queue->head != NULL) {
        sem->queue->head->prev = NULL;
    } else {
        sem->queue->tail = NULL;
    }
    pmm->free(node);
    return task;
}
void kmt_sem_init(sem_t *sem, const char *name, int value) {
    TRACE_ENTRY;
    sem->name = name;
    sem->value = value;
    char dst[256] = "";
    snprintf(dst, strlen(name)+4+1, "sem-%s", name);
    _spin_init(&sem->lk, dst);
    sem->queue = NULL;
    TRACE_EXIT;
}

// 怎么这里进了两次这个函数？观察一下cpu
// 都是cpu0上的，cnm我现在只启动了一个cpu，肯定是0
void kmt_sem_wait(sem_t *sem) { //666忘记实现这个了，难怪
    TRACE_ENTRY;
    INTR; // 果然，这里中断是关掉的，然后再上锁就会有问题
    // 稳定复现了，问题就是这个函数
    _spin_lock(&sem->lk); // 锁这个信号量加上自旋锁cpu
    // log("after spinlock\n");
    sem->value--;
    if (sem->value < 0) {
        log("if\n");
        // 当前线程不能执行，BLOCKED！
        current->status = BLOCKED; //TODO: 检查线程切换的函数，一会再看看
        sem_queue_push(sem, current); // 是不是这里上锁导致的
        _spin_unlock(&sem->lk);
        INTR;
    } else {
        log("else\n");
        _spin_unlock(&sem->lk);
        log("sem unlock\n");
        INTR;
        // 就是需要yield()出去的！
        yield(); // 不是你的问题
    }
    TRACE_EXIT;
}

void kmt_sem_signal(sem_t *sem) {
    TRACE_ENTRY;
    INTR;
    _spin_lock(&sem->lk);
    if (sem->value < 0) {
        PANIC_ON(sem->queue == NULL, "queue err in sem:%s", sem->name);
        task_t *task = sem_queue_pop(sem);
        PANIC_ON(task->status != BLOCKED, "blocked err");
        task->status = RUNNABLE;
    }
    sem->value++;
    _spin_unlock(&sem->lk);
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