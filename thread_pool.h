#ifndef __THREAD_POOL_H_
#define __THREAD_POOL_H

#define error(errString) fprintf(stderr, strcat("Error: ", errString))
#define JQ_NOTINIT "The Job Queue has not been Initialized"
#define MALLOC_FAIL "Could not allocate memory on Heap"
#define JOB_NOTINIT "Job provided has not been Initialized"
#define UNXPCTD_NULL "Unexpected NULL pointer found"
#define FREE_FAIL "Could not free memory at current location"
#define THPOOL_SZ_OOR "Invalid Pool Size (0 < numThreads < 512)"
#define THPOOL_NOTINIT "The ThreadPool has not been Initialized"
#define NOTSPWND_THREAD "Error During Thread Creation"

#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

enum ERRTYPE {ENULLARG = -1, EFREEFAIL = -2};

typedef struct Job {
    unsigned jobId;
    struct Job* nextJob;   
    void  (*jobRoutine)(void* arg);
    void* routineArgs;
} Job;

typedef struct JobQueue {
    pthread_mutex_t queueMutex;
    Job* queueHead;
    Job* queueTail;
    unsigned length;
} JobQueue;

typedef struct Thread {
    unsigned threadID;
    pthread_t thread;
} Thread;

typedef struct ThreadPool {
    Thread** pooledThreads;
    JobQueue* jobQueue;
    volatile int numAliveThreads;
    volatile int numWorkingThreads;
    pthread_mutex_t threadPoolMutex;
    pthread_cond_t putOnHold;
} ThreadPool;

/*space for functions prototypes*/

JobQueue* initAJobQueue();
int pushJob(JobQueue* jobQueue, Job* newJob);
Job* takeJob(JobQueue* jobQueue);
int clearJobQueue(JobQueue* jobQueue);
int disposeJobQueue(JobQueue* jobQueue);
void printJobQueue(const JobQueue* jobQueue);

Thread* spawnThread(unsigned threadID);
ThreadPool* initAThreadPool(unsigned poolSize);

#endif