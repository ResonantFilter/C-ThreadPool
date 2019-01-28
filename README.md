# TPool-C
Simple implementation of a Thread Pool using pthread.h

# Status
  Suffers from starvation but it is starting to work as expected..

# Compiling
compile using:
```console
  $ gcc -ansi -std=c11 -Wall -lpthread thread_pool.c job_queue.c threadPoolTest.c
```
