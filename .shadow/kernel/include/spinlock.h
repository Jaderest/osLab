#include <am.h>
#include <os.h>
#include <klib.h>
#include <klib-macros.h>

#define UNLOCKED  0
#define LOCKED    1

// 这里在kernel.h中定义了spinlock_t
struct spinlock {
    const char *name;
    int status;
    struct cpu *cpu;
};

extern struct cpu cpus[];
#define mycpu (&cpus[cpu_current()])

void _spin_lock(spinlock_t *lk);
void _spin_unlock(spinlock_t *lk);
