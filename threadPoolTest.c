#include "thread_pool.h"

int main() {
    ThreadPool* pool = initAThreadPool(512);
    free(pool);

    return 0;
}