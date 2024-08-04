#include <os.h>

static spinlock_t task_lock = spinlock_init("task_lock");

//TODO: check whether right
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
    task = pmm->alloc(sizeof(task_t));
    task->name = name;
    task->entry = entry;
    task->arg = arg;
    task->status = RUNNABLE;
    init_stack_guard(task);
    
    PANIC_ON(!stack_check(task), "Stack overflow");
    _spin_unlock(&task_lock);
    return 0;
}

void _teardown(task_t *task) {
    
}