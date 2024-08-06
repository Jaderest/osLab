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



// 保存context
Context *kmt_context_save(Event ev, Context *ctx) {
    return NULL;
}

Context *kmt_schedule(Event ev, Context *ctx) {
    return ctx;
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
            return 0;
        }
    }
    return 1;
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
        currents[i]->context = kcontext((Area) {currents[i]->stack, currents[i]->stack + STACK_SIZE}, NULL, NULL);
        // 试一下只有这个几个空转会不会出问题
        init_stack_guard(&idle[i]);
        PANIC_ON(stack_check(&idle[i]) == 0, "stack overflow in ");
    }
}

void kmt_init() {
    os->on_irq(INT_MIN, EVENT_NULL, kmt_context_save);
    os->on_irq(INT_MAX, EVENT_NULL, kmt_schedule);
    idle_init();
}

// task的内存已预先分配好，并且允许任何线程调用task_create
int kmt_create(task_t *task, const char *name, void (*entry)(void *arg), void *arg) {
    TRACE_ENTRY;
    _spin_lock(&task_lk); // 保护全局变量

    task_init(task, name);
    Area stack = (Area) {task->stack, task->stack + STACK_SIZE};

    task->context = kcontext(stack, entry, arg);

    NO_INTR;
    tasks[total_task_num] = task;
    total_task_num++;
    NO_INTR;

    _spin_unlock(&task_lk);
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