#include <os.h>

//TODO: check whether right
void init_stack_guard(task_t *task) {
    for (int i = 0; i < STACK_GUARD_SIZE; i++) {
        task->tsk.stack_guard_s[i] = STACK_GUARD_VALUE;
        task->tsk.stack_guard_e[i] = STACK_GUARD_VALUE;
    }
}

int _create(task_t *task, const char *name, void (*entry)(void *arg), void *arg) {
    task = pmm->alloc(TASK_STACK_SIZE);
    task->tsk.name = name;
    task->tsk.entry = entry;
    task->tsk.arg = arg;
    task->tsk.status = RUNNABLE;
    task->tsk.context = *kcontext (
        (Area) {
            .start = &(task->tsk.end),
            .end = &(task->tsk.end) + TASK_STACK_SIZE + 2*4*STACK_GUARD_SIZE,
        },
        entry, arg
    );
    // 如何检查堆栈？栈溢出的后果就是线程的信息可能被覆盖，出现各种诡异的情况
    return 0;
}

void _teardown(task_t *task) {
    
}