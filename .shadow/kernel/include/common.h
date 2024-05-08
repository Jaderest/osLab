#include <kernel.h>
#include <klib.h>
#include <klib-macros.h>

// #define LOCKED 0
// #define UNLOCKED 1

// typedef struct {
//     int locked;
// } lock_t;

// void lock_init(lock_t *lock) {
//     lock->locked = UNLOCKED;
// }

// void lock(lock_t *lock) {
//     while (atomic_xchg(&lock->locked, LOCKED) != UNLOCKED) ;
// }

// void unlock(lock_t *lock) {
//     atomic_xchg(&lock->locked, UNLOCKED);
// }