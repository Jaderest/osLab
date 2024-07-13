// DO NOT MODIFY: Will be reverted by the Online Judge.

#include <kernel.h>
#include <klib.h>

void alignTest() {
    for (const char *s = "Hello World from CPU #*\n"; *s; s++) {
        putch(*s == '*' ? '0' + cpu_current() : *s);
    }

    for (int i = 0; i < 119; i ++) {
        void *p = pmm->alloc(16+i);
        printf("p: %x\n", p);
    }

    while (1) ;
}

int main() {
    os->init();
    mpe_init(os->run);
    // mpe_init(alignTest);
    return 1;
}
