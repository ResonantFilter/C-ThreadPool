# TPool-C
Simple implementation of a Thread Pool using pthread.h

### Status
It now works as a simple task pusher/puller, minor bugs to be fixed.

## Functions Provided
```C
/*Returns the Pointer to a new Allocated ThreadPool of size 'poolSize'*/
ThreadPool* initAThreadPool(unsigned poolSize);

/*Submits a new Task to the ThreadPool given as argument.
The Task must be a Function Pointer casted to (void*) and returning NULL, 
where routineArgs must be provided throug a custom struct casted to void* containing 
all the parameters the Task might need.*/
void submitJob(ThreadPool* threadPool, void (*jobRoutine)(void*), void* routineArgs); 

/*(Under Improvement) Releases all the ThreadPool resources*/
void disposeThreadPool(ThreadPool* threadPool);
```

## Compiling
compile using:
```console
  $ gcc -ansi -std=c11 -Wall -lpthread thread_pool.c job_queue.c
```
### Example
```C
#include <time.h>
#include "thread_pool.h"
//gcc -ansi -std=c11 -Wall -lpthread thread_pool.c job_queue.c TPoolTest_longCycles.c -o TPoolTest_longCycles

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
    
    /*
    * Setup...
    */
    signal(SIGINT, sigHandler);

    ThreadPool* pool = initAThreadPool(NUMTHREADS);
    if (pool == NULL) {
        return -1;
    }

    while (running) {
        int x = rand() % 500;
        struct arg* a = (struct arg*)malloc(sizeof(struct arg)); 
        unsigned int v = (unsigned int) (800 + x);
        a->value = v;
        submitJob(pool, (void *)cycles, (void *)a);
        //sleep(1);
    }

    disposeThreadPool(pool);
    printf("Exiting...\n");
    return 0;
}
```
