#include "thread_pool.h"

void* print_ciao(void* arg) {
    printf("ciao\n");
    return NULL;
}

void* print_num(void* num) {
    printf("%x\n", *(int *)num);
    return NULL;
}

void* cycles(void* num) {
    unsigned cycles = (*(int *)num) * (*(int *)num) * (*(int *)num);
    for (unsigned i = 0; i < cycles; ++i);
    printf("made %d cycles\n", cycles);
    return NULL;
}

int main() {
    
    Job* jj1 = (Job *)malloc(sizeof(Job));
    jj1->jobId = 11;
    jj1->jobRoutine = (void *)print_ciao;
    jj1->routineArgs = NULL;

    Job* jj2 = (Job *)malloc(sizeof(Job));
    jj2->jobId = 22;
    jj2->jobRoutine = (void *)print_num;
    jj2->routineArgs = (void *)2;

    Job* jj3 = (Job *)malloc(sizeof(Job));
    jj3->jobId = 33;
    jj3->jobRoutine = (void *)cycles;
    jj3->routineArgs = (void *)12345;


    JobQueue* jobQueue = initAJobQueue();

    pushJob(jobQueue, jj1);
    pushJob(jobQueue, jj2);
    pushJob(jobQueue, jj3);

    while(jobQueue->length) {
        Job* j = takeJob(jobQueue);

        pthread_t p1;

        pthread_create(&p1, NULL, (void *)j->jobRoutine, &j->routineArgs);
        pthread_join(p1, NULL);
    }

    free(jj1);
    free(jj2);
    free(jj3);
    disposeJobQueue(jobQueue);

    return 0;   
}