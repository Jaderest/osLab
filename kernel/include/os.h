// When we test your kernel implementation, the framework
// directory will be replaced. Any modifications you make
// to its files (e.g., kernel.h) will be lost. 

// Note that some code requires data structure definitions 
// (such as `sem_t`) to compile, and these definitions are
// not present in kernel.h. 

// Include these definitions in os.h.

#ifndef __OS_H__
#define __OS_H__

#include <common.h>

struct cpu {
    int noff; // number of off（关中断的次数）
    int intena; // interrupt enable（中断是不是处于enable的状态）
};

extern struct cpu cpus[];
#define mycpu (&cpus[cpu_current()])


//------------------spinlock------------------
struct spinlock {
    const char *name;
    int status;
    struct cpu *cpu;
};

typedef struct task_node {
    task_t *task;
    struct task_node *prev;
    struct task_node *next;
} task_node_t;
typedef struct task_queue {
    task_node_t *head;
    task_node_t *tail;
} task_queue_t;

typedef struct {
    spinlock_t spinlock;
    int locked; // 表示锁是否被占用 or 被别的线程持有
    task_queue_t *wait_list;
} mutexlock_t;

#define spinlock_init(name_) \
    ((spinlock_t) { \
        .name = name_, \
        .status = UNLOCKED, \
        .cpu = NULL, \
    })
#define UNLOCKED  0
#define LOCKED    1
bool holding(spinlock_t *lk);
void _spin_lock(spinlock_t *lk);
void _spin_unlock(spinlock_t *lk);
void _spin_init(spinlock_t *lk, const char *name);
void mutex_init(mutexlock_t *lock, const char *name);
void mutex_lock(mutexlock_t *lock);
void mutex_unlock(mutexlock_t *lock);

//------------------task------------------
typedef enum {
    BLOCKED = 1,
    RUNNABLE, //2
    RUNNING,
} task_status_t;

struct task {
    const char *name;
    int id;
    int cpu_id; // debug need
    task_status_t status;
    Context *context; // 指针
    uint32_t stack_fense_s[STACK_GUARD_SIZE];
    uint8_t stack[STACK_SIZE];
    uint32_t stack_fense_e[STACK_GUARD_SIZE];
};

void init_stack_guard(task_t *task);
int check_stack_guard(task_t *task);
void idle_init(); // cpu 上空转的任务
// 写在kmt里，然后这里声明一下
Context *kmt_context_save(Event ev, Context *context);
Context *kmt_schedule(Event ev, Context *context);
int kmt_create(task_t *task, const char *name, void (*entry)(void *arg), void *arg);
void kmt_teardown(task_t *task);


//------------------semaphore------------------
/*
value指定了
*/

struct semaphore {
    // mutexlock_t lk;
    spinlock_t lk;
    int value; //0（生产者消费者缓冲区），1（互斥锁）
    const char *name;
    task_queue_t *queue; //TODO: 思考这里的list怎么管理
};


#endif // _OS_H__