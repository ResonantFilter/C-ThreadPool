#include <time.h>
#include "thread_pool.h"
//gcc -ansi -std=c11 -Wall -lpthread thread_pool.c job_queue.c threadPoolTest.c -o threadPoolTest


void* cycles(void* num) {
    unsigned _cycles = (*(int *)num) * (*(int *)num) * (*(int *)num);
    for (unsigned i = 0; i < _cycles; ++i);
    printf("made %d cycles\n", _cycles);
    return NULL;
}

int main() {
    srand(time(NULL));
    ThreadPool* pool = initAThreadPool(4);
    if (pool == NULL) {
        error("main.pool = initAthreadPool(4)-");
        error(THPOOL_SZ_OOR);
        return -1;
    }

    int i = 0;
    while (i < 100000) {
        int x = rand() % 100;
        submitJob(pool, (void *)cycles, (void *)1000 + x);
        i++;
    }

    return 0;
}