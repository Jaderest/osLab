// DO NOT MODIFY: Will be reverted by the Online Judge.

#include <kernel.h>
#include <klib.h>

void alignTest() {
    for (int i = 0; i < 119; i ++) {
        pmm->alloc(16+i);
    }
    while (1) ;
}

int main() {
    os->init();
    mpe_init(os->run);
    // mpe_init(alignTest);
    return 1;
}
