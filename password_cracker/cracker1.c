/**
 * password_cracker
 * CS 241 - Spring 2021
 */
#include "cracker1.h"
#include "format.h"
#include "utils.h"
#include "./includes/queue.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <crypt.h>


static queue* q;
static size_t num_tasks = 0;
static size_t num_success = 0;
static size_t num_fail = 0;
static pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;

void parse_input() {
    char* buffer = NULL;
    size_t capacity = 0;
    while (getline(&buffer, &capacity, stdin) != -1) {
        //get the input
        if (strlen(buffer) > 0 && buffer[strlen(buffer) - 1] == '\n') {
            buffer[strlen(buffer) - 1] = '\0';
        }
        char* temp = strdup(buffer);
        queue_push(q, temp);
        num_tasks++;
	}
    free(buffer);
}

void* crack(void* p) {
    int threadId = (long)p;
    pthread_mutex_lock(&m);
    while (num_tasks != 0) {
        char *line = queue_pull(q);
        num_tasks--;
        pthread_mutex_unlock(&m);
        char name[32], hash[32], knownPassword[32];

        //get name, hash, and password
        sscanf(line, "%s %s %s", name, hash, knownPassword);
        v1_print_thread_start(threadId, name);
        double begin_time = getThreadCPUTime();

        //set all the '.' letters to 'a'
        char* password = knownPassword + getPrefixLength(knownPassword);
        setStringPosition(password, 0);

        struct crypt_data cdata;
        cdata.initialized = 0;
        char *hashed = NULL;
        size_t num_hashes = 0;
        int res = 1;
        //try different passwords
        while (1) {
            num_hashes++;
            hashed = crypt_r(knownPassword, "xx", &cdata);
            if (!strcmp(hashed, hash)) {
                //find match
                pthread_mutex_lock(&m);
                num_success++;
                pthread_mutex_unlock(&m);
                res = 0;
                break;
            }
            if (incrementString(password) == 0) {
                //fail to find
                pthread_mutex_lock(&m);
                num_fail++;
                pthread_mutex_unlock(&m);
                break;
            }
        }
        v1_print_thread_result(threadId, name, knownPassword, num_hashes, getThreadCPUTime() - begin_time, res);
        free(line);
        pthread_mutex_lock(&m);
    }
    pthread_mutex_unlock(&m);
    return NULL;
}

int start(size_t thread_count) {
    // TODO your code here, make sure to use thread_count!
    // Remember to ONLY crack passwords in other threads
    pthread_t tid[thread_count];
    q = queue_create(-1);
    parse_input();

    for (size_t i = 0; i < thread_count; i++) {
        pthread_create(tid + i, NULL, crack, (void*)i+1);
    }
    for (size_t i = 0; i < thread_count; i++) {
        pthread_join(tid[i], NULL);
    }
    v1_print_summary(num_success, num_fail);
    pthread_mutex_destroy(&m);
    queue_destroy(q);
    return 0; // DO NOT change the return code since AG uses it to check if your
              // program exited normally
}
