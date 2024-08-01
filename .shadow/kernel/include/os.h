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

struct task {

};

struct semaphore {
    
};


#endif // _OS_H__