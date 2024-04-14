struct co* co_start(const char *name, void (*func)(void *), void *arg);
void co_yield();
void co_wait(struct co *co);
void traverse();
// void detect();
// void detect2();
// void detect3();
// void show_status();