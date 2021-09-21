/**
 * deadlock_demolition
 * CS 241 - Spring 2021
 */
#include "libdrm.h"

#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static drm_t *drm1;
static drm_t *drm2;
static drm_t *drm3;

void* myfunc1(void* p) {
    int res = 0;
    pthread_t *thread_id = (pthread_t*)pthread_self();
    res = drm_wait(drm1, thread_id);
    printf("%s: %d\n", "myfunc1", res);
    res = drm_wait(drm2, thread_id);
    printf("%s: %d\n", "myfunc1", res);
    res = drm_post(drm2, thread_id);
    printf("%s: %d\n", "myfunc1", res);
    res = drm_post(drm1, thread_id);
    printf("%s: %d\n", "myfunc1", res);
    return NULL;
}

void* myfunc2(void* p) {
    int res = 0;
    pthread_t *thread_id = (pthread_t*)pthread_self();
    res = drm_wait(drm2, thread_id);
    printf("%s: %d\n", "myfunc2", res);
    res = drm_wait(drm1, thread_id);
    printf("%s: %d\n", "myfunc2", res);
    res = drm_post(drm1, thread_id);
    printf("%s: %d\n", "myfunc2", res);
    res = drm_post(drm2, thread_id);
    printf("%s: %d\n", "myfunc2", res);
    return NULL;
}

void* myfunc3(void* p) {
    int res = 0;
    pthread_t *thread_id = (pthread_t*)pthread_self();
    res = drm_wait(drm3, thread_id);
    printf("%s: %d\n", "myfunc3", res);
    res = drm_wait(drm3, thread_id);
    printf("%s: %d\n", "myfunc3", res);
    res = drm_post(drm3, thread_id);
    printf("%s: %d\n", "myfunc3", res);
    res = drm_post(drm3, thread_id);
    printf("%s: %d\n", "myfunc3", res);
    return NULL;
}

int main() {
    pthread_t tid1;
    pthread_t tid2;
    pthread_t tid3;
    drm1 = drm_init();
    drm2 = drm_init();
    drm3 = drm_init();
    // TODO your tests here
    pthread_create(&tid1, 0, myfunc1, NULL);
    pthread_create(&tid2, 0, myfunc2, NULL);
    pthread_create(&tid3, 0, myfunc3, NULL);
    pthread_join(tid1,NULL);
    pthread_join(tid2,NULL);
    pthread_join(tid3,NULL);
    drm_destroy(drm1);
    drm_destroy(drm2);
    drm_destroy(drm3);
    return 0;
}
