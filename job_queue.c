#include "thread_pool.h"

void clearJobs(JobQueue* jobQueue); //Auxiliary function

JobQueue* initAJobQueue() {
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

int pushJob(JobQueue* jobQueue, Job* newJob) {
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

    newJob->nextJob = NULL;
    if (jobQueue->length == 0) {
        pthread_mutex_lock(&(jobQueue->queueMutex));
            jobQueue->queueHead = newJob;
            jobQueue->queueTail = newJob;           
            ++jobQueue->length;
            setGreenToOneThread(jobQueue->syncSem);
            //printf("l=0, sent GREEN signal\n");
        pthread_mutex_unlock(&(jobQueue->queueMutex));
        
        return 0;
    }

    pthread_mutex_lock(&(jobQueue->queueMutex));        
        jobQueue->queueTail->nextJob = newJob;
        jobQueue->queueTail = newJob;
        ++jobQueue->length;
        setGreenToOneThread(jobQueue->syncSem);
        //printf("l>0, sent GREEN signal\n");
    pthread_mutex_unlock(&(jobQueue->queueMutex));

    return 0;
}

Job* takeJob(JobQueue* jobQueue) {
    if (jobQueue == NULL) {
        error("job_queue.takeJob.jobQueue-");
        error(JQ_NOTINIT);
        return NULL;
    }

    if (!jobQueue->length) {
        error("No more jobs to do!\n");
        exit(0);
    }

    if (jobQueue->length == 1) {
        pthread_mutex_lock(&(jobQueue->queueMutex));
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

    pthread_mutex_lock(&(jobQueue->queueMutex));
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

Job* peekJob(const JobQueue *jobQueue) {
    return !(jobQueue->queueHead) ? NULL : jobQueue->queueHead;    
}

int clearJobQueue(JobQueue* jobQueue) {
    clearJobs(jobQueue);    
    jobQueue = initAJobQueue();
    return 0;
}

int disposeJobQueue(JobQueue* jobQueue) {
    clearJobs(jobQueue);
    free(jobQueue);
    return 0;
}

void printJobQueue(const JobQueue *jobQueue) {
    if (!jobQueue) return;
    printf("length: %d\n", jobQueue->length);
    Job* aux = jobQueue->queueHead;

    while (aux != NULL) {
        printf("job: %d\n", aux->jobId);
        aux = !(aux->nextJob) ? NULL : aux->nextJob;
    }
}

void clearJobs(JobQueue* jobQueue) {
    if (jobQueue == NULL) {
        error("job_queue.clearJobs.jobQueue-");
        error(JQ_NOTINIT);
        return;
    }

    while (jobQueue->length) {
        free(takeJob(jobQueue));
    }
}

//Semaphore functions

Semaphore* initASemaphore(Binary value) {
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

void setGreenToOneThread(Semaphore* syncSem) {
    pthread_mutex_lock(&syncSem->semMutex);
        syncSem->value = GREEN;
        pthread_cond_signal(&syncSem->semCondition);
    pthread_mutex_unlock(&syncSem->semMutex);        
}

void setGreenToAllThreads(Semaphore* syncSem) {
    pthread_mutex_lock(&syncSem->semMutex);
        syncSem->value = GREEN;
        pthread_cond_broadcast(&syncSem->semCondition);
    pthread_mutex_unlock(&syncSem->semMutex);
}

void waitForGreenLight(Semaphore* syncSem) {
    pthread_mutex_lock(&syncSem->semMutex);
        while (syncSem->value == RED) {
            pthread_cond_wait(&syncSem->semCondition, &syncSem->semMutex);
        }
        syncSem->value = RED;
    pthread_mutex_unlock(&syncSem->semMutex);
}