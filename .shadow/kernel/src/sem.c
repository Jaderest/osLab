#include <os.h>

void _sem_init(sem_t *sem, const char *name, int value) {
    sem->name = "sem"; // 这里不能直接赋值诶
    //TODO name：name是否需要分配一个新的空间
    sem->value = value;
    sem->lk = spinlock_init(name); //TODO：检查这里lk是否赋值成功
}

// P 操作
void _sem_wait(sem_t *sem) {
    _spin_lock(&sem->lk);
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
void _sem_signal(sem_t *sem) {
    _spin_lock(&sem->lk);
    
    _spin_unlock(&sem->lk);
}