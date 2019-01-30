# TPool-C
Simple implementation of a Thread Pool using pthread.h

# Status
  It now works as a simple task push/pull, with bugs appearing from time to time

# Compiling
compile using:
```console
  $ gcc -ansi -std=c11 -Wall -lpthread thread_pool.c job_queue.c threadPoolTest.c
```
