/**
 * teaching_threads
 * CS 241 - Spring 2021
 */
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "reduce.h"
#include "reducers.h"

/* You might need a struct for each task ... */
typedef struct task_ {
    int* list;
    reducer reduce_func;
    size_t length;
    size_t idx;
} task_t;
/* You should create a start routine for your threads. */

static int* res;

void* run(void* ptr) {
    task_t* task = ptr;
    int* list = task->list;
    size_t length = task->length;
    size_t idx = task->idx;
    reducer reduce_func = task->reduce_func;
    if (length == 1) {
        res[idx] = list[0];
        pthread_exit(NULL);
    }
    int result = reduce(list + 1, length - 1, reduce_func, list[0]);
    res[idx] = result;
    pthread_exit(NULL);
}

int par_reduce(int *list, size_t list_len, reducer reduce_func, int base_case,
               size_t num_threads) {
    /* Your implementation goes here */
    if (num_threads > list_len) {
        num_threads = list_len;
    }
    pthread_t tid[num_threads];
    res = calloc(num_threads, sizeof(int));
    task_t* tasks = calloc(num_threads, sizeof(task_t));
    size_t task_len = list_len / num_threads;   //length of the list for each thread except the last one
    for (size_t i = 0; i < num_threads; i++) {
        //init task
        if (i != num_threads - 1) {
            tasks[i].length = task_len;
        } else {
            tasks[i].length = list_len - task_len * i;
        }
        tasks[i].list = calloc(tasks[i].length, sizeof(int));
        tasks[i].idx = i;
        tasks[i].reduce_func = reduce_func;
        for (size_t j = 0; j < tasks[i].length; j++) {
            (tasks[i].list)[j] = list[i * task_len + j];
        }
        //pthread_create
        pthread_create(tid + i, NULL, run, tasks + i);
    }
    for (size_t i = 0; i < num_threads; i++) {
        //pthread_join
        pthread_join(tid[i], NULL);
    }
    //obtain result
    int result = reduce(res, num_threads, reduce_func, base_case);
    //free memory
    free(res);
    res = NULL;
    for (size_t i = 0; i < num_threads; i++) {
        free(tasks[i].list);
        tasks[i].list = NULL;
    }
    free(tasks);
    tasks = NULL;
    return result;
}
