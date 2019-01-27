#include "thread_pool.h"

void* cycles(void* num) {
    unsigned _cycles = (*(int *)num) * (*(int *)num) * (*(int *)num);
    for (unsigned i = 0; i < _cycles; ++i);
    printf("made %d cycles\n", _cycles);
    return NULL;
}

int main() {

    ThreadPool* pool = initAThreadPool(4);
    if (pool == NULL) {
        error(THPOOL_SZ_OOR);
        return -1;
    }

    submitJob(pool, (void *)cycles, (void *)1234);
    submitJob(pool, (void *)cycles, (void *)4321);
    submitJob(pool, (void *)cycles, (void *)6789);

    pauseThreadPool(pool);

    free(pool);

    return 0;
}