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
#define THREAD_NOTINIT  "The Thread has not been Initialized"

#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

enum ERRTYPE {ENULLARG = -1, EFREEFAIL = -2};
typedef struct Job Job;
typedef struct JobQueue JobQueue;
typedef struct Thread Thread;
typedef struct ThreadPool ThreadPool;

typedef struct Job {
    unsigned jobId;
    struct Job* nextJob;   /*TODO*/
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
    ThreadPool* fatherPool; //ThreadPool to which the Thread belongs
    pthread_t thread;
} Thread;

typedef struct ThreadPool {
    Thread** pooledThreads;
    JobQueue* jobQueue;
    volatile int numAliveThreads;
    volatile int numWorkingThreads;
    pthread_mutex_t threadPoolMutex;
    pthread_cond_t startWorking;
} ThreadPool;

/*space for functions prototypes*/

JobQueue* initAJobQueue();
int pushJob(JobQueue* jobQueue, Job* newJob);
Job* takeJob(JobQueue* jobQueue);
int clearJobQueue(JobQueue* jobQueue);
int disposeJobQueue(JobQueue* jobQueue);
void printJobQueue(const JobQueue* jobQueue);

Thread* spawnThread(ThreadPool* fatherPool ,unsigned threadID);
ThreadPool* initAThreadPool(unsigned poolSize);
void* doWork(void* _worker);

#endif