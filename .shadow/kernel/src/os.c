#include <common.h>

#ifdef TEST
#include <am.h>
#include <stdio.h>
void putch(char ch) {
    putchar(ch);
}
#endif

static void os_init() {
    pmm->init();
}

static void os_run() {
    for (const char *s = "Hello World from CPU #*\n"; *s; s++) {
        putch(*s == '*' ? '0' + cpu_current() : *s);
    }
    while (1) ;
}

MODULE_DEF(os) = {
    .init = os_init,
    .run  = os_run,
};
