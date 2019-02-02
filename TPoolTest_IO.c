#include "thread_pool.h"
#include <string.h>
//gcc -ansi -std=c11 -Wall -lpthread thread_pool.c job_queue.c TPoolTest_IO.c -o TPoolTest_IO

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
        fprintf(stderr, "No such file\n");
        return NULL;
    }
    
    fseek(fPointer, 0, SEEK_END);
    long fileLength = ftell(fPointer);
    fseek(fPointer, 0, SEEK_SET);
    
    char* readText = (char*)malloc(fileLength);
    if (!readText) {
        fprintf(stderr, "Could not read file's content\n");
        return NULL;
    }
    
    fread(readText, 1, fileLength, fPointer);
    if (fPointer) fclose(fPointer);
    printf("%s\n", readText);
    printf("Done..\n");
    return NULL;
}

int main(int argc, char** argv) {
    int numThreads;
    if (argc < 2) {
        error("Usage: ./TPoolTest_IO numThreads\n");
        return -1;
    } else {
        numThreads = strtol(argv[1], NULL, 10);
    }

    const int NUMTHREADS = numThreads;

    srand(time(NULL));
    signal(SIGINT, sigHandler);

    ThreadPool* pool = initAThreadPool(NUMTHREADS);
    struct arg* arg_v = (struct arg* )malloc(sizeof(struct arg));
    if (!arg_v) return 0;

    while(running) {
        sleep(2);
        
        struct arg* arg_v1 = (struct arg* )malloc(sizeof(struct arg));
        if (!arg_v1) return 0;
        char* c1 = "./.gitignore";
        arg_v1->path = c1;
        submitJob(pool, (void*)readFile, (void *)arg_v1);
        
        struct arg* arg_v2 = (struct arg* )malloc(sizeof(struct arg));
        if (!arg_v2) return 0;        
        char* c2 = "./thread_pool.c";
        arg_v2->path = c2;
        submitJob(pool, (void*)readFile, (void *)arg_v2);        
        
        struct arg* arg_v3 = (struct arg* )malloc(sizeof(struct arg));
        if (!arg_v3) return 0;
        char* c3 = "./TPoolTest_IO.c";
        arg_v3->path = c3;
        submitJob(pool, (void*)readFile, (void *)arg_v3);
        
        struct arg* arg_v4 = (struct arg* )malloc(sizeof(struct arg));
        if (!arg_v4) return 0;
        char* c4 = "./TPoolTest_longCycles.c";
        arg_v4->path = c4;
        submitJob(pool, (void*)readFile, (void *)arg_v4);
    }   

    disposeThreadPool(pool);

    return 0;
}