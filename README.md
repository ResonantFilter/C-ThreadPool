# TPool-C
Simple implementation of a Thread Pool using pthread.h.
Made for educational porpuses only, it does not mean to be an example of fast performances and/or correctness.

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
  $ gcc -ansi -std=c11 -Wall -lpthread -c thread_pool.c
```
### Examples

##### Example with compute-bound operations
We submit to our ThreadPool a Task which computes fairly long empty for-loops.

```C
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
        if (!a) {
            return -1;
        }
        unsigned int v = (unsigned int) (800 + x);
        a->value = v;
        submitJob(pool, (void *)cycles, (void *)a);
    }

    disposeThreadPool(pool);
    printf("Exiting...\n");
    return 0;
}
```
##### Example with IO-bound operations
We submit to our ThreadPool a Task which reads a bunch of text files and prints them to stdout.

```C
#include "thread_pool.h"
#include <string.h>
//gcc -ansi -std=c11 -Wall -lpthread thread_pool.c TPoolTest_IO.c -o TPoolTest_IO

static volatile int running = 1;

void sigHandler(int signum) {
    running = 0;
}

struct arg {
    const char* path;
};

void* readFile(const void* const argv) {
    if (!argv) {
        return NULL;
    }
    struct arg a = *(struct arg*)argv;
    const char* const inputFileName = a.path;
    
    FILE* fPointer = fopen(inputFileName, "r");
    if (!fPointer) {
        fprintf(stderr, "%s, No such file\n", inputFileName);
        return NULL;
    }
    
    fseek(fPointer, 0, SEEK_END);
    long fileLength = ftell(fPointer);
    fseek(fPointer, 0, SEEK_SET);
    
    char* readText = (char*)malloc((size_t) fileLength);
    if (!readText) {
        fprintf(stderr, "Could not read file's content\n");
        return NULL;
    }
    
    if (fread(readText, 1, fileLength, fPointer) != fileLength) {
        error("fread encountered an error!\n");
        return NULL;
    }

    if (fPointer) fclose(fPointer);
    printf("%s\n", readText);
    printf("Done..\n");
    return NULL;
}

int main(int argc, char** argv) {
    /* 
    * Setup...
    */

    ThreadPool* pool = initAThreadPool(NUMTHREADS);
    struct arg* arg_v = (struct arg* )malloc(sizeof(struct arg));
    if (!arg_v) return -1;

    while(running) {
        printf("adding...\n");  
        struct arg* arg_v1 = (struct arg* )malloc(sizeof(struct arg));
        if (!arg_v1) return 0;
        char* c1 = "./.gitignore";
        arg_v1->path = c1;
        submitJob(pool, (void*)readFile, (void *)arg_v1);
        
        struct arg* arg_v2 = (struct arg* )malloc(sizeof(struct arg));
        if (!arg_v2) return 0;        
        char* c2 = "./README.md";
        arg_v2->path = c2;
        submitJob(pool, (void*)readFile, (void *)arg_v2);        

        struct arg* arg_v3 = (struct arg* )malloc(sizeof(struct arg));
        if (!arg_v3) return 0;        
        char* c3 = "./TPoolTest_IO.c";
        arg_v3->path = c3;
        submitJob(pool, (void*)readFile, (void *)arg_v3);        
    }   

    disposeThreadPool(pool);

    return 0;
}
```
