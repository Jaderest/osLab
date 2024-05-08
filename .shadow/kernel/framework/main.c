// DO NOT MODIFY: Will be reverted by the Online Judge.

#include <kernel.h>
#include <klib.h>

void alignTest() {
    for (int i = 0; i < 10; i ++) {
        pmm->alloc(32+i);
    }
}

int main() {
    os->init();
    // mpe_init(os->run);
    mpe_init(alignTest);
    return 1;
}
