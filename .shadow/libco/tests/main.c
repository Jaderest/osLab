#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "co-test.h"

int g_count = 0;

static void add_count() {
    g_count++;
}

static int get_count() {
    return g_count;
}

static void work_loop(void *arg) {
    const char *s = (const char*)arg;
    for (int i = 0; i < 100; ++i) {
        printf("%s%d  ", s, get_count());
        add_count();
        co_yield();
    }
}

static void work(void *arg) {
    work_loop(arg);
}

static void test_1() {

    struct co *thd1 = co_start("thread-1", work, "X");
    struct co *thd2 = co_start("thread-2", work, "Y");
    traverse();

    co_wait(thd1);
    co_wait(thd2);

//    printf("\n");
}

// -----------------------------------------------

static int g_running = 1;

static void do_produce(Queue *queue) {
    assert(!q_is_full(queue));
    Item *item = (Item*)malloc(sizeof(Item));
    if (!item) {
        fprintf(stderr, "New item failure\n");
        return;
    }
    item->data = (char*)malloc(10);
    if (!item->data) {
        fprintf(stderr, "New data failure\n");
        free(item);
        return;
    }
    memset(item->data, 0, 10);
    sprintf(item->data, "libco-%d", g_count++);
    q_push(queue, item);
}

static void producer(void *arg) {
    Queue *queue = (Queue*)arg;
    for (int i = 0; i < 100; ) {
        if (!q_is_full(queue)) {
            // co_yield();
            do_produce(queue);
            i += 1;
        }
        // printf("i: %d\n", i);
        co_yield();
    }
}

static void do_consume(Queue *queue) {
    assert(!q_is_empty(queue));

    Item *item = q_pop(queue);
    if (item) {
        printf("%s  ", (char *)item->data);
        free(item->data);
        free(item);
    }
}

static void consumer(void *arg) {
    Queue *queue = (Queue*)arg;
    while (g_running) {
        if (!q_is_empty(queue)) {
            do_consume(queue);
        }
        // printf("consumer\n"); // 定位到你了
        co_yield();
    }
}

static void test_2() {

    Queue *queue = q_new();

    struct co *thd1 = co_start("producer-1", producer, queue);
    struct co *thd2 = co_start("producer-2", producer, queue);
    struct co *thd3 = co_start("consumer-1", consumer, queue);
    struct co *thd4 = co_start("consumer-2", consumer, queue);
    // 那就是start在此时没有好好被加入链表中
    // traverse();

    // printf("start producer\n");
    // traverse();
    co_wait(thd1); //其实这里两个 producer 都运行完了

    // printf("start producer2\n");
    // traverse();
    co_wait(thd2); // 这里不小心提前删掉了thd1？
    // printf("finish producer\n");


    g_running = 0;
    // printf("start consumer\n");
    // traverse();

    co_wait(thd3);
    co_wait(thd4);
    // printf("finish consumer\n");

    while (!q_is_empty(queue)) {
        do_consume(queue);
    }

    q_free(queue);
}

void print(void *arg) {
    const char *s = (const char*)arg;
    printf("arg: %s\n", s);
}

static void test_3() {
    struct co *thd1 = co_start("print-1", print, "X");
    struct co *thd2 = co_start("print-2", print, "Y");

    co_wait(thd1);
    co_wait(thd2);
}

int count = 1;

void entry(void *arg) {
    for (int i = 0; i < 5; i++) {
        printf("%s[%d] ", (const char *)arg, count++);
        co_yield();
    }
}

static void test_4() {
    struct co *thd1 = co_start("co-1", entry, "a");
    struct co *thd2 = co_start("co-2", entry, "b");

    co_wait(thd1);
    co_wait(thd2);
}

static void test_5() {
    struct co *thd[127];
    for (int i = 0; i < 127; i++) {
        thd[i] = co_start("co", entry, "a");
    }
    co_wait(thd[0]);
    printf("--------------------\n");

    for (int i = 1; i < 127; i++) {
        co_wait(thd[i]);
    }
}



static void test_6() {
    Queue *queue = q_new();

    struct co *thd[127];
    for (int i = 0; i < 64; i++) {
        thd[i] = co_start("producer", producer, queue);
    }

    for (int i = 64; i < 127; i++) {
        thd[i] = co_start("consumer", consumer, queue);
    }

    for (int i = 0; i < 64; i++) {
        co_wait(thd[i]);
    }

    g_running = 0;

    for (int i = 64; i < 127; i++) {
        co_wait(thd[i]);
    }

    while (!q_is_empty(queue)) {
        do_consume(queue);
    }

    q_free(queue);
}

void test_7() {
    co_yield();

    struct co *thd1 = co_start("print-1", print, "X");
    struct co *thd2 = co_start("print-2", print, "Y");

    co_wait(thd1);
    co_wait(thd2);
}

void test_8() {
    struct co *thd1 = co_start("print-1", print, "X");
    
    co_wait(thd1);
}

void test_9() {
    struct co *thd[127];
    for (int i = 0; i < 127; ++i) {
        char *name = (char *)malloc(64);
        sprintf(name, "thread-%d", i);
        thd[i] = co_start("co", work, "X");
    }

    for (int i = 0; i < 127; ++i)
        co_wait(thd[i]);
}

int main() {
    setbuf(stdout, NULL);

    printf("Test #1. Expect: (X|Y){0, 1, 2, ..., 199}\n");
    test_1();

    printf("\n\nTest #2. Expect: (libco-){200, 201, 202, ..., 399}\n");
    test_2();
    printf("\n\n");

    // printf("Test #3. My test to run them\n");
    // test_3();
    // printf("\n\n");

    // printf("Test #4. My test to run them\n");
    // test_4();
    // printf("\n\n");

    // printf("Test #5. My test to run them\n");
    // test_5();
    // printf("\n\n");

    // printf("Test #6. My test to run them\n");
    // test_6();
    // printf("\n\n");

    // printf("Test #7. My test to run them\n");
    // test_7();
    // printf("\n\n");

    // printf("Test #8. My test to run them\n");
    // test_8();
    // printf("\n\n");

    // printf("Test #9. My test to run them\n");
    // test_9();
    // printf("\n\n");
    return 0;
}
