#include "thread_pool.h"

volatile int onpause = 0;
static void pauseThread(int signum);
void pausePool(ThreadPool* threadPool);

void submitJob(ThreadPool* threadPool, void (*jobRoutine)(void*), void* routineArgs) {
    if (!threadPool->jobQueue) {
        error("thread_pool.submitJob.threadPool->jobQueue-");
        error(JQ_NOTINIT);
        return;
    }
    printf("jobqueue length: %d\n", threadPool->jobQueue->length);
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
        //sleep(5);
    }
}

void* doWork(void* _worker) {
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
        error("after wait\n");
        pthread_mutex_lock(&fatherPool->threadPoolMutex);
            ++fatherPool->numWorkingThreads;
        pthread_mutex_unlock(&fatherPool->threadPoolMutex);
        error("after ++\n");
        Job* currentJob = takeJob(fatherPool->jobQueue);
        printf("JOBADDR: %p\n", currentJob);
        error("Job taken\n");
        if (currentJob) {
            printf("Worker %d, job %d taken\n", worker->threadID, currentJob->jobId);
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

Thread* spawnThread(ThreadPool* fatherPool, unsigned threadID) {
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
        printf("Spawned Thread no. %d\n", threadPool->pooledThreads[i]->threadID);
    }

    while(threadPool->numAliveThreads != threadPool->poolSize);

    return threadPool;
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

static void pauseThread(int signum) {
    onpause = 1;
    while(onpause) {
        sleep(1);
    }
}

void pausePool(ThreadPool* threadPool) {
    for (int i = 0; i < threadPool->poolSize; ++i) {
        pthread_kill(threadPool->pooledThreads[i]->thread, SIGUSR1);
    }

}

void pauseThreadPool(ThreadPool* threadPool) {
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
}