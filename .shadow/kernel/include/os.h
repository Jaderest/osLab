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

//------------------task------------------
typedef enum {
    BLOCKED,
    RUNNABLE,
    RUNNING,
    DEAD,
} task_status_t;

struct task {
    const char *name;
    int id; // id 编号
    task_status_t status;
    struct task *next; 
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
typedef struct task_node {
    task_t *task;
    struct task_node *prev;
    struct task_node *next;
} task_node_t;
typedef struct task_queue {
    task_node_t *head;
    task_node_t *tail;
} task_queue_t;

struct semaphore {
    spinlock_t lk;
    int value; //0（生产者消费者缓冲区），1（互斥锁）
    const char *name;
    task_queue_t *queue; //TODO: 思考这里的list怎么管理
};
void _sem_init(sem_t *sem, const char *name, int value);
void _sem_wait(sem_t *sem);
void _sem_signal(sem_t *sem);










#endif // _OS_H__