#include <time.h>
#include "thread_pool.h"
//gcc -ansi -std=c11 -Wall -lpthread thread_pool.c job_queue.c TPoolTest_longCycles.c -o TPoolTest_longCycles

static volatile int running = 1;

void sigHandler(int signum) {
    printf("Exiting...\n");
    running = 0;
    exit(0);
}

void* cycles(void* _num) {
    unsigned num = *(int *)_num;
    unsigned _cycles = num * num * num;
    for (unsigned i = 0; i < _cycles; ++i);
    printf("made %u cycles\n", _cycles);
    return NULL;
}

int main(int argc, char** argv) {
    int numThreads;
    if (argc < 2) {
        error("Usage: ./TPoolTest_longCycles numThreads\n");
        return -1;
    } else {
        numThreads = strtol(argv[1], NULL, 10);
    }

    const int NUMTHREADS = numThreads;

    srand(time(NULL));
    signal(SIGINT, sigHandler);

    ThreadPool* pool = initAThreadPool(NUMTHREADS);
    if (pool == NULL) {
        error("main.pool = initAthreadPool-");
        error(THPOOL_SZ_OOR);
        return -1;
    }

    while (running) {
        int x = rand() % 500;
        submitJob(pool, (void *)cycles, (void *)800 + x);
        sleep(1);
    }
    return 0;
}