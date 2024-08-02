#include <os.h>
#include <stdint.h>
#include <klib.h>
#include <klib-macros.h>

struct cpu cpus[MAX_CPU_NUM];

void kmt_init() {

}

// 为task这个指针创建空间
/*
create 在系统中创建一个线程（task_t应当实现被分配好），这个线程立即就可以被调度执行
但是只有打开中断时它才获得被调度执行的权利，（关中断就让它等着）
然后它创建的线程永不返回，直到调用teardown
只有永远不会被调度到处理器上执行的前提才能被回收
static inline task_t *task_alloc() {
    return pmm->alloc(sizeof(task_t));
}

static void run_test1() {
    kmt->sem_init(&empty, "empty", N);
    kmt->sem_init(&fill,  "fill",  0);
    for (int i = 0; i < NPROD; i++) {
        kmt->create(task_alloc(), "producer", T_produce, NULL);
    }
    for (int i = 0; i < NCONS; i++) {
        kmt->create(task_alloc(), "consumer", T_consume, NULL);
    }
}
*/
int kmt_create(task_t *task, const char *name, void (*entry)(void *arg), void *arg) {
    return 0;
}

void kmt_teardown(task_t *task) {

}

void kmt_spin_init(spinlock_t *lk, const char *name) {
    //FIXME: 这里并不能使用alloc？
    //TODO: 思考一下要不要alloc
    lk->name = name;
    lk->status = UNLOCKED;
    lk->cpu = NULL;
}

void kmt_spin_lock(spinlock_t *lk) {
    _spin_lock(lk);
}

void kmt_spin_unlock(spinlock_t *lk) {
    _spin_unlock(lk);
}

void kmt_sem_init(sem_t *sem, const char *name, int value) {
    
}

void kmt_sem_wait(sem_t *sem) {
    
}

void kmt_sem_signal(sem_t *sem) {
    
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