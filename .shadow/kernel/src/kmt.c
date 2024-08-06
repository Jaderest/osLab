#include <os.h>
#include <stdint.h>
#include <klib.h>
#include <klib-macros.h>

struct cpu cpus[MAX_CPU_NUM];

Context *kmt_context_save(Event ev, Context *ctx) {
    return NULL;
}

Context *kmt_schedule(Event ev, Context *ctx) {
    return NULL;
}

void kmt_init() {
    os->on_irq(INT_MIN, EVENT_NULL, kmt_context_save);
    os->on_irq(INT_MAX, EVENT_NULL, kmt_schedule);
}

int kmt_create(task_t *task, const char *name, void (*entry)(void *arg), void *arg) {
    // return _create(task, name, entry, arg);
    return 0;
}

// 这个是不是也要注册成处理函数
void kmt_teardown(task_t *task) {
    // _teardown(task);
}

void kmt_spin_init(spinlock_t *lk, const char *name) {
    _spin_init(lk, name);
}

void kmt_spin_lock(spinlock_t *lk) {
    _spin_lock(lk);
}

void kmt_spin_unlock(spinlock_t *lk) {
    _spin_unlock(lk);
}

void kmt_sem_init(sem_t *sem, const char *name, int value) {
    // _sem_init(sem, name, value);
}

void kmt_sem_wait(sem_t *sem) {
    // _sem_wait(sem);
}

void kmt_sem_signal(sem_t *sem) {
    // _sem_signal(sem);
}




MODULE_DEF(kmt) = {
    .init = kmt_init,
    .create = kmt_create,
    .teardown = kmt_teardown,
    .spin_init = kmt_spin_init,
    .spin_lock = kmt_spin_lock,
    .spin_unlock = kmt_spin_unlock,
    .sem_init = kmt_sem_init,
    .sem_wait = kmt_sem_wait,
    .sem_signal = kmt_sem_signal,
};