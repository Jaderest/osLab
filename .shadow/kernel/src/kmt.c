#include <os.h>
#include <stdint.h>
#include <klib.h>
#include <klib-macros.h>

struct cpu cpus[MAX_CPU_NUM];

#define MAX_TASK_NUM 128 // 最多支持128个任务
static task_t idle[MAX_CPU_NUM];     // cpu 上空转的任务
static task_t *currents[MAX_CPU_NUM]; // 当前任务
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

    current->status = RUNNABLE; // 当前任务切换为可执行，初始情况其实是设置的idle，但是idle不在task队列里面
    // 于是在schedule时可以assert检查idle
    current->context = ctx; // 保存当前的context

    stack_check(current);
    NO_INTR;
    return NULL;
}

//idle 应该写错了
Context *kmt_schedule(Event ev, Context *ctx) { // ?理一下思路先，不急着跑代码
    // 获取可以运行的任务
#ifdef  MONITOR
    if (cpu_current() == cpu_count() - 1) { //  单独针对这个cpu
        log("--------monitor-------\n");
        for (int i = 0; i < cpu_count(); ++i) {
            log("monitor:cpu %d: %s\n", i, currents[i]->name);
        }
        for (int i = 0; i < total_task_num; ++i) {
            log("monitor:task %d: %s status = %d\n", i, tasks[i]->name, tasks[i]->status);
        }
        current = &idle[cpu_current()];
        current->status = RUNNING;
        log("--------monitor-------\n");
        return current->context;
    }
#endif
    NO_INTR;
    _spin_lock(&task_lk); // ？你不是上锁了吗怎么数据竞争了
    stack_check(current);

    int index = current->id; // 从当前任务开始
    int i = 0;
    for (i = 0; i < total_task_num * 10; ++i) { // 循环十遍
        index = (index + 1) % total_task_num; // index也跟着循环
        if (tasks[index] == NULL) { //TODO: teardown
            continue;
        }
        if (tasks[index]->status == RUNNABLE) {
            current = tasks[index];
            current->status = RUNNING;
            break;
        }
    }
    NO_INTR; // 这里也不应该啊
    // 处理获取结果
    //FIXME: 嘻嘻
    PANIC_ON(!holding(&task_lk), "cnm"); // 这里应该是持有任务这把锁的???
    if (i == total_task_num * 10) {
        PANIC_ON(idle[cpu_current()].status != RUNNABLE, "idle err in cpu %d", cpu_current());
        current = &idle[cpu_current()];
        current->status = RUNNING;
        log("%s idle on cpu %d\n", current->name, cpu_current());
    } else {
        current = tasks[index];
        current->status = RUNNABLE;
        log("%s not idle on cpu %d\n", current->name, cpu_current());
    }

    _spin_unlock(&task_lk);
    NO_INTR;
    // 不是，这个task怎么回事
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
    // _teardown(task);
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
void kmt_sem_init(sem_t *sem, const char *name, int value) {
    // _sem_init(sem, name, value);
}

void kmt_sem_wait(sem_t *sem) {
    // _sem_wait(sem);
}

void kmt_sem_signal(sem_t *sem) {
    // _sem_signal(sem);
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