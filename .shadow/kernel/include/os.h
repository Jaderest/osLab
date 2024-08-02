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
    int noff;
    int intena;
};

extern struct cpu cpus[];
#define mycpu (&cpus[cpu_current()])


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

// 需要为这个结构体分配内存（想想需要什么内容）
struct task {

};

struct semaphore {
    
};

//------------------spinlock------------------
#define UNLOCKED  0
#define LOCKED    1
void _spin_lock(spinlock_t *lk);
void _spin_unlock(spinlock_t *lk);

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