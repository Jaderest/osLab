#include <os.h>

void _sem_init(sem_t *sem, const char *name, int value) {
    sem = pmm->alloc(sizeof(sem_t));
    sem->name = name;
    sem->value = value;
    // TODO: 更新sem数据结构
    sem->queue = NULL;
    sem->lk = spinlock_init(name); //TODO：检查这里lk是否赋值成功
}

// P 操作
/*
允许在线程中执行信号量的 P，但是P操作没有相应资源时，线程将被阻塞（不再被调度执行，？这里是不再还是可以恢复成其他状态）
中断没有对应的线程，不能阻塞，因此不能在中断时调用 P
*/
void _sem_wait(sem_t *sem) {
    _spin_lock(&sem->lk); //即使用锁阻塞在这里
    /*
    if (...) {
      // 没有资源，需要等待
      ...
      mark_as_not_runnable(current); //标志当前线程不能再执行
    }
    */
    _spin_unlock(&sem->lk);
    /*
    if (...) { // 若 P 操作失败，则不能继续执行
      // 有可能会有线程执行 V 操作
      ...
      yield(); 
    }
    */
}

// V 操作
/*
允许任意状态下执行V，包括任何处理器中的任何线程，任何处理器的任何中断
*/
void _sem_signal(sem_t *sem) {
    _spin_lock(&sem->lk);
    
    _spin_unlock(&sem->lk);
}