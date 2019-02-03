#include "thread_pool.h"

static JobQueue* initAJobQueue();
static int pushJob(JobQueue* jobQueue, Job* newJob);
static Job* takeJob(JobQueue* jobQueue);
static Job* peekJob(const JobQueue *jobQueue);
static void clearJobs(JobQueue* jobQueue); //Auxiliary function
static int disposeJobQueue(JobQueue* jobQueue);
static Semaphore* initASemaphore(Binary value);
static void waitForGreenLight(Semaphore* syncSem);
static void setGreenToOneThread(Semaphore* syncSem);
static Thread* spawnThread(ThreadPool* fatherPool, unsigned threadID);
static void* doWork(void* _worker);
static volatile int onpause = 0;
static void pauseThread(int signum);

/* TODO FUNCTIONS */
//JOBQUEUE
//static int clearJobQueue(JobQueue* jobQueue);
//static void printJobQueue(const JobQueue *jobQueue);
//static void setGreenToAllThreads(Semaphore* syncSem);

//THREADPOOL
//static void pauseThreadPool(ThreadPool* threadPool);

/* TODO FUNCTIONS */
//static void pausePool(ThreadPool* threadPool);

/* <STATIC FUNCTIONS DECLARATIONS. FOR LOCAL USE ONLY> */

/* ThreadPool static Functions */

static void* doWork(void* _worker) {
    if (_worker == NULL) {
        error("thread_pool.doWork._worker-");
        error(THPOOL_NOTINIT);
        pthread_exit((void *)ENULLARG);
    }

    Thread* worker = (Thread *)_worker;
    ThreadPool* fatherPool = worker->fatherPool;

    pthread_mutex_lock(&fatherPool->threadPoolMutex);
        ++fatherPool->numAliveThreads;
    pthread_mutex_unlock(&fatherPool->threadPoolMutex); 

    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sa.sa_handler = pauseThread;

    if (sigaction(SIGUSR1, &sa, NULL)) {
        error("Unable to Catch 'SIGUSR1' signal\n");
    }

    while(1) {
        waitForGreenLight(fatherPool->jobQueue->syncSem);        
        
        pthread_mutex_lock(&fatherPool->threadPoolMutex);
            ++fatherPool->numWorkingThreads;
        pthread_mutex_unlock(&fatherPool->threadPoolMutex);
        
        Job* currentJob = takeJob(fatherPool->jobQueue);
        if (currentJob) {
            //printf("Worker %d, job %d taken\n", worker->threadID, currentJob->jobId);
            void (*routine)(void*);
            void *_routineArgs;
            routine =  (void *)currentJob->jobRoutine;
            _routineArgs = currentJob->routineArgs;
            if (routine == NULL || _routineArgs == NULL) {
                error("thread_pool.doWork.currentJob-");
                error(UNXPCTD_NULL);
            }
            routine(_routineArgs);
            free(currentJob);
        }  
    }

    pthread_mutex_lock(&fatherPool->threadPoolMutex);
            --fatherPool->numWorkingThreads;
    pthread_mutex_unlock(&fatherPool->threadPoolMutex);   

    return NULL; 
}

static Thread* spawnThread(ThreadPool* fatherPool, unsigned threadID) {
    if (fatherPool == NULL) {
        error("thread_pool.spawnThread.fatherPool-");
        error(THPOOL_NOTINIT);
        return NULL;
    }

    Thread* thread_str = (Thread *)malloc(sizeof(Thread));
    if (thread_str == NULL) {
        error("thread_pool.spawnThread.thread_str-");
        error(MALLOC_FAIL);
        return NULL;        
    }

    thread_str->threadID = threadID;
    thread_str->fatherPool = fatherPool;
    
    pthread_create(&(thread_str->thread), NULL, doWork, (void *)thread_str);
    pthread_detach(thread_str->thread);

    return thread_str;
}

static void pauseThread(int signum) {
    onpause = 1;
    while(onpause) {
        sleep(1);
    }
}

/*static void pausePool(ThreadPool* threadPool) {
    for (int i = 0; i < threadPool->poolSize; ++i) {
        pthread_kill(threadPool->pooledThreads[i]->thread, SIGUSR1);
    }
}*/

/*static void pauseThreadPool(ThreadPool* threadPool) {
    if (!threadPool) {
        error("thread_pool.pauseThreadPool.threadPool-");
        error(THPOOL_NOTINIT);
        return;
    }

    pthread_mutex_lock(&threadPool->threadPoolMutex);
        while (threadPool->jobQueue->length || threadPool->numWorkingThreads) {
            pthread_cond_wait(&threadPool->waitingForJobs, &threadPool->threadPoolMutex);
        }
    pthread_mutex_unlock(&threadPool->threadPoolMutex);
}*/

/* JobQueue static Functions */

static JobQueue* initAJobQueue() {
    JobQueue* jobQueue = (JobQueue *)malloc(sizeof(JobQueue));
    if (jobQueue == NULL) {
        error("job_queue.initAJobQueue.jobQueue-");
        error(MALLOC_FAIL);
        return NULL;
    }    
    jobQueue->length = 0;
    jobQueue->queueHead = NULL;
    jobQueue->queueTail = NULL;
    jobQueue->syncSem = initASemaphore(RED);

    pthread_mutex_init(&(jobQueue->queueMutex), NULL);

    return jobQueue;
}

static int pushJob(JobQueue* jobQueue, Job* newJob) {
    if (jobQueue == NULL) {
        error("job_queue.pushJob.jobQueue-");
        error(JQ_NOTINIT);
        return ENULLARG;
    }

    if (newJob == NULL) {
        error("job_queue.pushJob.newJob-");
        error(JOB_NOTINIT);
        return ENULLARG;
    }

    pthread_mutex_lock(&(jobQueue->queueMutex));

    if (jobQueue->length == 0) {
        jobQueue->queueHead = newJob;
        jobQueue->queueTail = newJob;
        newJob->nextJob = NULL;
        ++jobQueue->length;
        setGreenToOneThread(jobQueue->syncSem);
        pthread_mutex_unlock(&(jobQueue->queueMutex));
        return 0;
    }

    jobQueue->queueTail->nextJob = newJob;
    jobQueue->queueTail = newJob;
    newJob->nextJob = NULL;
    ++jobQueue->length;
    setGreenToOneThread(jobQueue->syncSem);
    pthread_mutex_unlock(&(jobQueue->queueMutex));
    return 0;
}

static Job* takeJob(JobQueue* jobQueue) {
    if (jobQueue == NULL) {
        error("job_queue.takeJob.jobQueue-");
        error(JQ_NOTINIT);
        return NULL;
    }

    pthread_mutex_lock(&jobQueue->queueMutex);
    
    if (peekJob(jobQueue) == NULL) {
        pthread_mutex_unlock(&jobQueue->queueMutex);
        return NULL;
    }

    if (jobQueue->length == 1) {
            Job* jobToBeTaken = jobQueue->queueHead;
            if (!jobToBeTaken) {
                error("job_queue.takeJob.jobToBeTaken [length = 1]-");
                error(UNXPCTD_NULL);
                return NULL;
            }
            jobQueue->queueHead = NULL;
            jobQueue->queueTail = NULL;
            jobQueue->length = 0;
        pthread_mutex_unlock(&(jobQueue->queueMutex));
        return jobToBeTaken;
    }

    Job* jobToBeTaken = jobQueue->queueHead;
    if (!jobToBeTaken) {
        error("job_queue.takeJob.jobToBeTaken [length > 1]-");
        error(UNXPCTD_NULL);
        return NULL;
    }
    jobQueue->queueHead = jobQueue->queueHead->nextJob;
    --jobQueue->length;
    
    setGreenToOneThread(jobQueue->syncSem);  
    pthread_mutex_unlock(&(jobQueue->queueMutex));
      
    return jobToBeTaken;    
}

static Job* peekJob(const JobQueue *jobQueue) {
    return !(jobQueue->queueHead) ? NULL : jobQueue->queueHead;    
}

/*static int clearJobQueue(JobQueue* jobQueue) {
    clearJobs(jobQueue);    
    jobQueue = initAJobQueue();
    return 0;
}*/

static int disposeJobQueue(JobQueue* jobQueue) {
    clearJobs(jobQueue);
    free(jobQueue->syncSem);
    free(jobQueue);
    return 0;
}

/*static void printJobQueue(const JobQueue *jobQueue) {
    if (!jobQueue) return;
    printf("length: %d\n", jobQueue->length);
    Job* aux = jobQueue->queueHead;

    while (aux != NULL) {
        printf("job: %d\n", aux->jobId);
        aux = !(aux->nextJob) ? NULL : aux->nextJob;
    }
}*/

static void clearJobs(JobQueue* jobQueue) {
    if (jobQueue == NULL) {
        error("job_queue.clearJobs.jobQueue-");
        error(JQ_NOTINIT);
        return;
    }

    while (jobQueue->length) {
        free(takeJob(jobQueue));
    }
}

/* Sync Semaphore static Functions */

static Semaphore* initASemaphore(Binary value) {
    if (value != RED && value != GREEN) {
        error(NOTA_SEMVALUE);
        return NULL;
    }

    Semaphore* syncSem = (Semaphore *)malloc(sizeof(Semaphore));
    if (syncSem == NULL) {
        error("job_queue.initASemaphore.syncSem-");
        error(MALLOC_FAIL);
        return NULL;
    }

    syncSem->value = value;
    pthread_mutex_init(&syncSem->semMutex, NULL);
    pthread_cond_init(&syncSem->semCondition, NULL);

    return syncSem;
}

static void setGreenToOneThread(Semaphore* syncSem) {
    pthread_mutex_lock(&syncSem->semMutex);
        syncSem->value = GREEN;
        pthread_cond_signal(&syncSem->semCondition);
    pthread_mutex_unlock(&syncSem->semMutex);        
}

/*static void setGreenToAllThreads(Semaphore* syncSem) {
    pthread_mutex_lock(&syncSem->semMutex);
        syncSem->value = GREEN;
        pthread_cond_broadcast(&syncSem->semCondition);
    pthread_mutex_unlock(&syncSem->semMutex);
}*/

static void waitForGreenLight(Semaphore* syncSem) {
    pthread_mutex_lock(&syncSem->semMutex);
        while (syncSem->value == RED) {
            pthread_cond_wait(&syncSem->semCondition, &syncSem->semMutex);
        }
        syncSem->value = RED;
    pthread_mutex_unlock(&syncSem->semMutex);
}

/* </STATIC FUNCTIONS DECLARATIONS. FOR LOCAL USE ONLY> */

/* EXPORTED METHODS TO BE USED BY USERS */

ThreadPool* initAThreadPool(unsigned poolSize) {
    if (poolSize < 0 || poolSize > 512) {
        error("thread_pool.initAThreadPool-");
        error(THPOOL_SZ_OOR);
        return NULL;
    }
    ThreadPool* threadPool = (ThreadPool *)malloc(sizeof(ThreadPool));
    if (threadPool == NULL) {
        error("thread_pool.initAThreadPool.threadPool-");
        error(MALLOC_FAIL);
        return NULL;
    }
    
    threadPool->poolSize = poolSize;
    threadPool->numAliveThreads = 0;
    threadPool->numWorkingThreads = 0;

    threadPool->jobQueue = initAJobQueue();
    if (threadPool->jobQueue == NULL) {
        error("thread_pool.initAThreadPool.jobQueue = initAJobQueue-");
        error(MALLOC_FAIL);
        free(threadPool);
        return NULL;
    }
    
    threadPool->pooledThreads = (Thread **)malloc(poolSize * sizeof(Thread*));
    if (threadPool->pooledThreads == NULL) {
        error("thread_pool.initAThreadPool.pooledThreads-");
        error(MALLOC_FAIL);
        disposeJobQueue(threadPool->jobQueue);
        free(threadPool);
        return NULL;
    }
    
    pthread_mutex_init(&(threadPool->threadPoolMutex), NULL);
    pthread_cond_init(&(threadPool->waitingForJobs), NULL);

    for (unsigned i = 0; i < poolSize; ++i) {
        threadPool->pooledThreads[i] = spawnThread(threadPool, i);
        if (threadPool->pooledThreads[i] == NULL) {
            error(NOTSPWND_THREAD);
            for (int j = 0; j < i; ++j) {
                free(threadPool->pooledThreads[j]);
            }
            disposeJobQueue(threadPool->jobQueue);
            free(threadPool);
        }
        //printf("Spawned Thread no. %d\n", threadPool->pooledThreads[i]->threadID);
    }

    while(threadPool->numAliveThreads != threadPool->poolSize);

    return threadPool;
}

void submitJob(ThreadPool* threadPool, void (*jobRoutine)(void*), void* routineArgs) {
    if (!threadPool->jobQueue) {
        error("thread_pool.submitJob.threadPool->jobQueue-");
        error(JQ_NOTINIT);
        return;
    }
    //printf("jobqueue length: %d\n", threadPool->jobQueue->length);
    static volatile int jobID = 0;
    if (!threadPool) {
        error("thread_pool.submitJob.threadPool-");
        error(THPOOL_NOTINIT);
        return;
    }
    const unsigned maxJobs = 100000;
    if (threadPool->jobQueue->length >= (threadPool->poolSize)*maxJobs) {
        while(threadPool->jobQueue->length > (threadPool->poolSize*(maxJobs*0.9)));
        return;
    }

    Job* newJob = (Job *)malloc(sizeof(Job));
    if (!newJob) {
        error("thread_pool.submitJob.newJob-");
        error(MALLOC_FAIL);
        return;
    }

    newJob->jobId = jobID++;
    newJob->jobRoutine = jobRoutine;
    newJob->routineArgs = routineArgs;
    //printf("routine addr: %p, args addr: %p\n", newJob->jobRoutine, newJob->routineArgs);

    if (pushJob(threadPool->jobQueue, newJob) < 0) {
        error("PushJob Failed to Execute");
    }
}

void disposeThreadPool(ThreadPool* threadPool) {
    printf("\nDisposing Resources..\n");
    if (threadPool == NULL) {
        error("thread_pool.disposeThreadPool.threadPool-");
        error(THPOOL_NOTINIT);
        return;
    }

    disposeJobQueue(threadPool->jobQueue);
    
    for (unsigned i = 0; i < threadPool->poolSize; ++i) {
        pthread_kill(threadPool->pooledThreads[i]->thread, SIGINT);
        free(threadPool->pooledThreads[i]);
    }
    free(threadPool->pooledThreads);

    free(threadPool);
}