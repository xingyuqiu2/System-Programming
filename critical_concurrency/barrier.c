/**
 * critical_concurrency
 * CS 241 - Spring 2021
 */
#include "barrier.h"

// The returns are just for errors if you want to check for them.
int barrier_destroy(barrier_t *barrier) {
    int error = 0;
    error = pthread_mutex_destroy(&(barrier->mtx));
    if (error != 0) return error;
    error = pthread_cond_destroy(&(barrier->cv));
    return error;
}

int barrier_init(barrier_t *barrier, unsigned int num_threads) {
    int error = 0;
    error = pthread_mutex_init(&(barrier->mtx), NULL);
    if (error != 0) return error;
    error = pthread_cond_init(&(barrier->cv), NULL);
    if (error != 0) return error;
    barrier->n_threads = num_threads;
    barrier->count = 0;
    barrier->times_used = 1;
    return error;
}

int barrier_wait(barrier_t *barrier) {
    pthread_mutex_lock(&(barrier->mtx));
    //prevent thread passing the next barrier while previous barrier hasn't finished
    while (barrier->times_used == 0) {
        pthread_cond_wait(&(barrier->cv), &(barrier->mtx));
    }
    barrier->count++;
    while (barrier->times_used == 1 && barrier->count != barrier->n_threads) {
        pthread_cond_wait(&(barrier->cv), &(barrier->mtx));
    }
    barrier->count--;
    if (barrier->count == 0) {
        //if all the threads leave the barrier, set times_used = 1
        barrier->times_used = 1;
    } else {
        //block the next barrier
        barrier->times_used = 0;
    }
    pthread_cond_broadcast(&(barrier->cv));
    pthread_mutex_unlock(&(barrier->mtx));
    return 0;
}
