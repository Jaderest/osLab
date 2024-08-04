#include <os.h>

static int total_nt = 0; // 任务总数
static task_t idle[MAX_CPU_NUM] = {};      // 每个cpu的idle task（空闲任务）
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
    PANIC_ON(!check_stack_guard(current), "Stack overflow detected in CPU #%d\n", cpu_current());

    if (current->status != BLOCKED) current->status = RUNNABLE;
    current->context = ctx;

    NO_INTR;
    TRACE_EXIT;
    return NULL;
}

Context *kmt_schedule(Event ev, Context *ctx) {
    TRACE_ENTRY;
    NO_INTR;

    int index = current->id;
    int i = 0;
    while (i < total_nt) {
        index = (index + 1) % total_nt;
        if (tasks[index]->status == RUNNABLE) break;
        i++;
    }
    if (i == total_nt) {
        current = &idle[cpu_current()]; // 初始化要创建这个东西
    } else {
        current = tasks[index];
    }
    current->status = RUNNING;

    NO_INTR;
    PANIC_ON(!check_stack_guard(current), "Stack overflow detected in CPU #%d\n", cpu_current());
    TRACE_EXIT;
    return current->context;
}