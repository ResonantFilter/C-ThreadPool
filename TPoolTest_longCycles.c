#include <time.h>
#include "thread_pool.h"
//gcc -ansi -std=c11 -Wall -lpthread thread_pool.c TPoolTest_longCycles.c -o TPoolTest_longCycles

static volatile int running = 1;

struct arg {
    unsigned value;
}; 

void sigHandler(int signum) {
    running = 0;
}

void* cycles(void* arg_v) {
    struct arg a = *(struct arg *)arg_v;
    unsigned num = a.value;
    unsigned _cycles = num * num * num;
    for (unsigned i = 0; i < _cycles; ++i);
    printf("made %u cycles\n", _cycles);
    return NULL;
}

int main(int argc, char** argv) {
    unsigned int numThreads;
    if (argc < 2) {
        error("Usage: ./TPoolTest_longCycles numThreads\n");
        return -1;
    } else {
        numThreads = (unsigned int) strtol(argv[1], NULL, 10);
    }

    const unsigned int NUMTHREADS = numThreads;

    srand((unsigned int) time(NULL));
    signal(SIGINT, sigHandler);

    ThreadPool* pool = initAThreadPool(NUMTHREADS);
    if (pool == NULL) {
        error("main.pool = initAthreadPool-");
        error(THPOOL_SZ_OOR);
        return -1;
    }

    while (running) {
        int x = rand() % 500;
        struct arg* a = (struct arg*)malloc(sizeof(struct arg)); 
        unsigned int v = (unsigned int) (800 + x);
        a->value = v;
        submitJob(pool, (void *)cycles, (void *)a);
    }

    disposeThreadPool(pool);
    printf("Exiting...\n");
    return 0;
}