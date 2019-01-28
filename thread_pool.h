#define _XOPEN_SOURCE //needed for using sigaction
#define _POSIX_C_SOURCE 200809L
#ifndef __THREAD_POOL_H_
#define __THREAD_POOL_H

#define error(errString) fprintf(stderr, errString)
#define JQ_NOTINIT "The Job Queue has not been Initialized\n"
#define MALLOC_FAIL "Could not allocate memory on Heap\n"
#define JOB_NOTINIT "Job provided has not been Initialized\n"
#define NOTA_SEMVALUE "Invalid Semaphore Assignement\n"
#define UNXPCTD_NULL "Unexpected NULL pointer found\n"
#define FREE_FAIL "Could not free memory at current location\n"
#define THPOOL_SZ_OOR "Invalid Pool Size (0 < numThreads < 512)\n"
#define THPOOL_NOTINIT "The ThreadPool has not been Initialized\n"
#define NOTSPWND_THREAD "Error During Thread Creation\n"
#define THREAD_NOTINIT  "The Thread has not been Initialized\n"

#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

enum ERRTYPE {ENULLARG = -1, EFREEFAIL = -2};
typedef struct Job Job;
typedef struct JobQueue JobQueue;
typedef struct Thread Thread;
typedef struct ThreadPool ThreadPool;
typedef struct Semaphore Semaphore;

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
    Semaphore* syncSem;
} JobQueue;

typedef struct Thread {
    unsigned threadID;
    ThreadPool* fatherPool; //ThreadPool to which the Thread belongs
    pthread_t thread;
} Thread;

typedef struct ThreadPool {
    Thread** pooledThreads;
    JobQueue* jobQueue;
    volatile unsigned numAliveThreads;
    volatile unsigned numWorkingThreads;
    unsigned poolSize;
    pthread_mutex_t threadPoolMutex;
    pthread_cond_t waitingForJobs;
} ThreadPool;

enum Binary {RED, GREEN};
typedef enum Binary Binary;
typedef struct Semaphore {
    Binary value;
    pthread_mutex_t semMutex;
    pthread_cond_t semCondition;    
} Semaphore;

/*space for functions prototypes*/

JobQueue* initAJobQueue();
int pushJob(JobQueue* jobQueue, Job* newJob);
Job* takeJob(JobQueue* jobQueue);
Job* peekJob(const JobQueue *jobQueue);
int clearJobQueue(JobQueue* jobQueue);
int disposeJobQueue(JobQueue* jobQueue);
void printJobQueue(const JobQueue *jobQueue);

Semaphore* initASemaphore(Binary value);
void waitForGreenLight(Semaphore* syncSem);
void setGreenToOneThread(Semaphore* syncSem);
void setGreenToAllThreads(Semaphore* syncSem);

Thread* spawnThread(ThreadPool* fatherPool, unsigned threadID);
ThreadPool* initAThreadPool(unsigned poolSize);
void* doWork(void* _worker);
void submitJob(ThreadPool* threadPool, void (*jobRoutine)(void*), void* routineArgs);
void pauseThreadPool(ThreadPool* threadPool);

#endif