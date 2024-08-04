#include <os.h>

// This is a ported version of spin-lock
// from xv6-riscv to AbstractMachine:
// https://github.com/mit-pdos/xv6-riscv

//! 这个spinlock是相当好的实现，我们写的时候也可以用它

void push_off(); // push_off相当于中断开和关
void pop_off();
bool holding(spinlock_t *lk);

void _spin_lock(spinlock_t *lk) {
    // Disable interrupts to avoid deadlock.关闭中断避免死锁
    push_off();

    // This is a deadlock.
    if (holding(lk)) { //如果上锁&持有锁的cpu为当前cpu，则立即终止它
        PANIC("acquire %s", lk->name);
    }

    // This our main body of spin lock.
    int got;
    do {
        got = atomic_xchg(&lk->status, LOCKED);
    } while (got != UNLOCKED);

    lk->cpu = mycpu;
}

void _spin_unlock(spinlock_t *lk) {
    if (!holding(lk)) { //看着怪怪的检查
        PANIC("release %s", lk->name);
    }

    lk->cpu = NULL;
    atomic_xchg(&lk->status, UNLOCKED);

    pop_off();
}

// Check whether this cpu is holding the lock.
// Interrupts must be off.
bool holding(spinlock_t *lk) {
    return (
        lk->status == LOCKED &&
        lk->cpu == &cpus[cpu_current()]
    );
}

// push_off/pop_off are like intr_off()/intr_on()
// except that they are matched:
// it takes two pop_off()s to undo two push_off()s.
// Also, if interrupts are initially off, then
// push_off, pop_off leaves them off.
void push_off(void) { //如何处理中断，关中断时，push中断前的状态放到栈里面
    int old = ienabled();
    iset(false); //关中断

    struct cpu *c = mycpu;
    if (c->noff == 0) { //number of 关中断的次数
        c->intena = old; //interrupt enable 中断是否处于打开的状态
    }
    c->noff += 1;
}

void pop_off(void) {
    struct cpu *c = mycpu;

    // Never enable interrupt when holding a lock.
    if (ienabled()) { // 解锁的时候中断是不是打开的
        panic("pop_off - interruptible");
    }
    
    if (c->noff < 1) {
        panic("pop_off");
    } // 许多正确的检查

    c->noff -= 1;
    if (c->noff == 0 && c->intena) {
        iset(true);
    }
}
