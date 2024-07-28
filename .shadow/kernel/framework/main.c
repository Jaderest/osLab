// DO NOT MODIFY: Will be reverted by the Online Judge.

#include <kernel.h>
#include <klib.h>

void alignTest1() {
    for (const char *s = "Hello World from CPU #*\n"; *s; s++) {
        putch(*s == '*' ? '0' + cpu_current() : *s);
    }

    // for (int i = 0; i < 4; i ++) {
    //     void *p = pmm->alloc(4096);
    //     p += 1;
    // }
    // for (int i = 0; i < 4; i ++) {
    //     void *p = pmm->alloc(128 + i);
    //     // printf("p: %x\n", p);
    // }

    while (1) ;
}

void alignTest2() {
    for (const char *s = "Hello World from CPU #*\n"; *s; s++) {
        putch(*s == '*' ? '0' + cpu_current() : *s);
    }

    for (int i = 0; i < 10; i ++) {
        void *p1 = pmm->alloc(5000);
        void *p2 = pmm->alloc(8200);
        printf("p1: %x\n", p1);
        printf("p2: %x\n", p2);
    }

    while (1) ;

}

int main() {
    os->init();
    // mpe_init(os->run);
    mpe_init(alignTest1);
    // mpe_init(alignTest2);
    return 1;
}