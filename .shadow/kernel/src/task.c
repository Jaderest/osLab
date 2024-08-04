#include <os.h>

int _create(task_t *task, const char *name, void (*entry)(void *arg), void *arg) {
    task = pmm->alloc(TASK_STACK_SIZE);
    task->tsk.name = name;
    task->tsk.entry = entry;
    task->tsk.arg = arg;
    task->tsk.status = RUNNABLE;
    task->tsk.context = *kcontext (
        (Area) {
            .start = &(task->tsk.end),
            .end = task->tsk.stack + TASK_STACK_SIZE,
        },
        entry, arg
    );
    // 如何检查堆栈？栈溢出的后果就是线程的信息可能被覆盖，出现各种诡异的情况
    //TODO: task的上下文需要如何创建
    return 0;
}

void _teardown(task_t *task) {
    
}