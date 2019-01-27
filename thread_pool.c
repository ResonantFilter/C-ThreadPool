#include "thread_pool.h"

void* doWork(void* _worker) {
    if (_worker == NULL) {
        error(THPOOL_NOTINIT);
        pthread_exit((void *)ENULLARG);
    }

    Thread* worker = (Thread *)_worker;
    ThreadPool* fatherPool = worker->fatherPool;

    pthread_mutex_lock(&fatherPool->threadPoolMutex);
        fatherPool->numAliveThreads += 1;
    pthread_mutex_unlock(&fatherPool->threadPoolMutex);

    pthread_exit(0);
}

Thread* spawnThread(ThreadPool* fatherPool ,unsigned threadID) {
    if (fatherPool == NULL) {
        error(THPOOL_NOTINIT);
        return NULL;
    }

    Thread* thread_str = (Thread *)malloc(sizeof(Thread));
    if (thread_str == NULL) {
        error(MALLOC_FAIL);
        return NULL;        
    }

    thread_str->threadID = threadID;
    thread_str->fatherPool = fatherPool;
    
    pthread_create(&(thread_str->thread), NULL, doWork, thread_str);
    pthread_detach(thread_str->thread);

    return thread_str;
}


ThreadPool* initAThreadPool(unsigned poolSize) {
    if (poolSize < 0 || poolSize > 512) {
        error(THPOOL_SZ_OOR);
        return NULL;
    }
    ThreadPool* threadPool = (ThreadPool *)malloc(sizeof(ThreadPool));
    if (threadPool == NULL) {
        error(MALLOC_FAIL);
    }
    
    threadPool->numAliveThreads = 0;
    threadPool->numWorkingThreads = 0;

    if ((threadPool->jobQueue = initAJobQueue()) == NULL) {
        error(MALLOC_FAIL);
        free(threadPool);
        return NULL;
    }
    
    threadPool->pooledThreads = (Thread **)malloc(poolSize * sizeof(Thread*));
    if (threadPool->pooledThreads == NULL) {
        error(MALLOC_FAIL);
        disposeJobQueue(threadPool->jobQueue);
        free(threadPool);
        return NULL;
    }
    
    pthread_mutex_init(&(threadPool->threadPoolMutex), NULL);
    pthread_cond_init(&(threadPool->startWorking), NULL);

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

    while(threadPool->numAliveThreads != poolSize);

    return threadPool;
}