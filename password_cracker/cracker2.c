/**
 * password_cracker
 * CS 241 - Spring 2021
 */
#include "cracker2.h"
#include "format.h"
#include "utils.h"
#include "./includes/queue.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <crypt.h>

static char name[32];
static char hash[32];
static char password[32];

static int stop = 0;    //flag indicating the end of input
static int find = 0;    //flag indicating the find of correct password
static int res = 1;     //res of the current password cracking after all threads have finished
static size_t thread_count = 0;
static int hashCount = 0;
static pthread_barrier_t barrier_start;
static pthread_barrier_t barrier_end;
static pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;

void* crack(void* p) {
    int threadId = (long)p;
    while (1) {
        //waiting for start
        pthread_barrier_wait(&barrier_start);

        pthread_mutex_lock(&m);
        if (stop) {
            //no more password to crack(end of input), should exit
            break;
        }
        char* cur_password = strdup(password);
        pthread_mutex_unlock(&m);

        struct crypt_data cdata;
        cdata.initialized = 0;
        char *hashed = NULL;

        long i = 0, count = 0;
        int num_hashes = 0, result = 2;
        int prefixLength = getPrefixLength(cur_password);
        getSubrange(strlen(cur_password) - prefixLength, thread_count, threadId, &i, &count);
        //set the cur_password to the assigned start password to crack
        char* unknown = cur_password + prefixLength;
        setStringPosition(unknown, i);

        v2_print_thread_start(threadId, name, i, cur_password);
        long end_i = i + count;
        for (; i < end_i; i++) {
            pthread_mutex_lock(&m);
            if (find) {
                //the thread was stopped early because the password was cracked by another thread
                pthread_mutex_unlock(&m);
                result = 1;
                break;
            }
            pthread_mutex_unlock(&m);

            num_hashes++;
            hashed = crypt_r(cur_password, "xx", &cdata);
            if (!strcmp(hashed, hash)) {
                //the thread has successfully cracked the password
                pthread_mutex_lock(&m);
                res = 0;
                find = 1;
                size_t idx = 0;
                for (; idx < strlen(cur_password); idx++) {
                    password[idx] = cur_password[idx];
                }
                password[idx] = 0;
                pthread_mutex_unlock(&m);
                result = 0;
                break;
            }
            if (incrementString(unknown) == 0) break;
        }

        v2_print_thread_result(threadId, num_hashes, result);
        free(cur_password);
        pthread_mutex_lock(&m);
        hashCount += num_hashes;
        pthread_mutex_unlock(&m);
        //waiting for the finish of all threads on current password
        pthread_barrier_wait(&barrier_end);
    }
    pthread_mutex_unlock(&m);
    return NULL;
}

int start(size_t thread_count_in) {
    // TODO your code here, make sure to use thread_count!
    // Remember to ONLY crack passwords in other threads
    thread_count = thread_count_in;
    pthread_barrier_init(&barrier_start, NULL, thread_count + 1);
    pthread_barrier_init(&barrier_end, NULL, thread_count + 1);
    pthread_t tid[thread_count];

    for (size_t i = 0; i < thread_count; i++) {
        pthread_create(tid + i, NULL, crack, (void*)i+1);
    }

    char* buffer = NULL;
    size_t capacity = 0;
    while (getline(&buffer, &capacity, stdin) != -1) {
        //get the input
        if (strlen(buffer) > 0 && buffer[strlen(buffer) - 1] == '\n') {
            buffer[strlen(buffer) - 1] = '\0';
        }
        char* line = strdup(buffer);
        //get name, hash, and password
        sscanf(line, "%s %s %s", name, hash, password);

        v2_print_start_user(name);
        double begin_time = getTime();
        double begin_CPUtime = getCPUTime();
        //start the cracking of all the other threads
        pthread_barrier_wait(&barrier_start);
        //wait for other threads to finish cracking
        pthread_barrier_wait(&barrier_end);
        v2_print_summary(name, password, hashCount, getTime() - begin_time, getCPUTime() - begin_CPUtime, res);

        //reset the variables
        pthread_mutex_lock(&m);
        find = 0;
        res = 1;
        hashCount = 0;
        pthread_mutex_unlock(&m);
        free(line);
	}
    free(buffer);
    pthread_mutex_lock(&m);
    stop = 1;
    pthread_mutex_unlock(&m);
    //call wait barrier_start so that other threads can receive the stop flag
    pthread_barrier_wait(&barrier_start);

    for (size_t i = 0; i < thread_count; i++) {
        pthread_join(tid[i], NULL);
    }
    pthread_mutex_destroy(&m);
    pthread_barrier_destroy(&barrier_start);
    pthread_barrier_destroy(&barrier_end);
    return 0; // DO NOT change the return code since AG uses it to check if your
              // program exited normally
}
