#include <os.h>

#define MAX_THREAD 256

static int total_nt = 0; // 任务总数
static task_t *tasks[MAX_THREAD] = {};
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
#define stack_check(task) check_stack_guard(task)


int _create(task_t *task, const char *name, void (*entry)(void *arg), void *arg) {
    _spin_lock(&task_lock);
    NO_INTR;

    task = pmm->alloc(sizeof(task_t));
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
    
}