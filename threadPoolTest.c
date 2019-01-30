#include <time.h>
#include "thread_pool.h"
//gcc -ansi -std=c11 -Wall -lpthread thread_pool.c job_queue.c threadPoolTest.c -o threadPoolTest

static volatile int running = 1;

void sigHandler(int signum) {
    printf("Exiting...\n");
    running = 0;
    exit(0);
}

void* cycles(void* num) {
    unsigned _cycles = (*(int *)num) * (*(int *)num) * (*(int *)num);
    for (unsigned i = 0; i < _cycles; ++i);
    printf("made %u cycles\n", _cycles);
    return NULL;
}

int main() {
    srand(time(NULL));
    signal(SIGINT, sigHandler);

    ThreadPool* pool = initAThreadPool(8);
    if (pool == NULL) {
        error("main.pool = initAthreadPool(4)-");
        error(THPOOL_SZ_OOR);
        return -1;
    }


    while (running) {
        int x = rand() % 500;
        submitJob(pool, (void *)cycles, (void *)1000 + x);
        sleep(1);
    }
    return 0;
}