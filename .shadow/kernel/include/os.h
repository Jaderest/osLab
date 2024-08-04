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
void _spin_lock(spinlock_t *lk);
void _spin_unlock(spinlock_t *lk);

//------------------semaphore------------------
//TODO SEMAPHORE: 
/*
value指定了
*/
struct semaphore {
    const char *name;
    int value; //0（生产者消费者缓冲区），1（互斥锁）
    //TODO: 这里需要一个队列(gpt) and 各种可能的成员
    spinlock_t lk;
};
void _sem_init(sem_t *sem, const char *name, int value);
void _sem_wait(sem_t *sem);
void _sem_signal(sem_t *sem);

//------------------task------------------
typedef enum {
    BLOCKED,
    RUNNABLE,
    RUNNING,
    ZOMBIE,
} task_status_t;

union tsk_union {
    struct {
        const char *name;
        void (*entry)(void *arg);
        void *arg;
        Context *context;
        task_status_t status;
        union tsk_union *next; //或许使用其他数据结构，或许是队列
        //TODO： 或许需要一个task的状态
        char end[0];
    };
    uint8_t stack[TASK_STACK_SIZE];
};

struct task {
    union tsk_union tsk;
};

int _create(task_t *task, const char *name, void (*entry)(void *arg), void *arg);
void _teardown(task_t *task);








//------------------log------------------
#define LOG
#ifdef LOG
extern spinlock_t log_lk;
#define log(format, ...) \
    do { \
        _spin_lock(&log_lk); \
        printf(format, ##__VA_ARGS__); \
        _spin_unlock(&log_lk); \
    } while (0)
#else
#define log(format, ...)
#endif

#endif // _OS_H__