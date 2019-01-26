#include "thread_pool.h"

void clearJobs(JobQueue* jobQueue); //Auxiliary function

JobQueue* initAJobQueue() {
    JobQueue* jobQueue = (JobQueue *)malloc(sizeof(JobQueue));
    if (jobQueue == NULL) {
        error(MALLOC_FAIL);
        return NULL;
    }    
    jobQueue->length = 0;
    jobQueue->queueHead = NULL;
    jobQueue->queueTail = NULL;
    pthread_mutex_init(&(jobQueue->queueMutex), NULL);

    return jobQueue;
}

int pushJob(JobQueue* jobQueue, Job* newJob) {
    if (jobQueue == NULL) {
        error(JQ_NOTINIT);
        return ENULLARG;
    }

    if (newJob == NULL) {
        error(JOB_NOTINIT);
        return ENULLARG;
    }

    newJob->nextJob = NULL;
    if (jobQueue->length == 0) {
        pthread_mutex_lock(&(jobQueue->queueMutex));
            jobQueue->queueHead = newJob;
            jobQueue->queueTail = newJob;           
            ++jobQueue->length;
        pthread_mutex_unlock(&(jobQueue->queueMutex));
        
        return 0;
    }

    pthread_mutex_lock(&(jobQueue->queueMutex));        
        jobQueue->queueTail->nextJob = newJob;
        jobQueue->queueTail = newJob;
        ++jobQueue->length;
    pthread_mutex_unlock(&(jobQueue->queueMutex));

    return 0;
}

Job* takeJob(JobQueue* jobQueue) {
    if (jobQueue == NULL) {
        error(JQ_NOTINIT);
        return NULL;
    }

    if (!jobQueue->length) return NULL;

    pthread_mutex_lock(&(jobQueue->queueMutex));
        Job* jobToBeTaken = jobQueue->queueHead;
        if (!jobToBeTaken) {
            error(UNXPCTD_NULL);
            return NULL;
        }
        jobQueue->queueHead = jobQueue->queueHead->nextJob;
        --jobQueue->length;
    pthread_mutex_unlock(&(jobQueue->queueMutex));
    
    return jobToBeTaken;    
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

void printJobQueue(const JobQueue* jobQueue) {
    printf("length: %d\n", jobQueue->length);
    Job* aux = jobQueue->queueHead;
    while (aux != NULL) {
        printf("job: %d\n", aux->jobId);
        aux = (aux->nextJob != NULL) ? aux->nextJob : NULL;
    }
}

void clearJobs(JobQueue* jobQueue) {
    if (jobQueue == NULL) {
        error(JQ_NOTINIT);
        return;
    }

    while (jobQueue->length) {
        free(takeJob(jobQueue));
    }
}