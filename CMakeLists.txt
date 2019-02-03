cmake_minimum_required(VERSION 2.6)
project(TPool-C)
configure_file(
        "${PROJECT_SOURCE_DIR}/thread_pool.h"
        "${PROJECT_BINARY_DIR}/thread_pool.h"
)

set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ansi -std=c11 -Wall -lpthread")

include_directories("${PROJECT_SOURCE_DIR}")
add_executable(TPoolTest_longCycles thread_pool.c job_queue.c TPoolTest_longCycles.c)
add_executable(TPoolTest_IO thread_pool.c job_queue.c TPoolTest_IO.c)