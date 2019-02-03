# TPool-C
Simple implementation of a Thread Pool using pthread.h

# Status
It now works as a simple task pusher/puller, minor bugs to be fixed.

# Functions Provided
```C
/*Returns the Pointer to a new Allocated ThreadPool of size 'poolSize'*/
ThreadPool* initAThreadPool(unsigned poolSize);

/*Submits a new Task to the ThreadPool given as argument, 
The Task must be a Function casted to (void*) and returning void*, 
where routineArgs must be provided throug a custom struct casted to void* containing 
all the parameters the Task might need.*/
void submitJob(ThreadPool* threadPool, void (*jobRoutine)(void*), void* routineArgs); 

/*(Under Improvement) Releases all the ThreadPool resources*/
void disposeThreadPool(ThreadPool* threadPool);
```

# Compiling
compile using:
```console
  $ gcc -ansi -std=c11 -Wall -lpthread thread_pool.c job_queue.c
```
