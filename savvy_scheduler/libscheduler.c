/**
 * savvy_scheduler
 * CS 241 - Spring 2021
 */
#include "libpriqueue/libpriqueue.h"
#include "libscheduler.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "print_functions.h"

/**
 * The struct to hold the information about a given job
 */
typedef struct _job_info {
    int id;

    /* TODO: Add any other information and bookkeeping you need into this
     * struct. */
    double arrival_time;
    double start_time;
    double required_run_time;
    double remaining_time;
    double rr_time;
    int priority;
} job_info;

static double total_waiting_time;
static double total_turnaround_time;
static double total_response_time;
static size_t num_jobs;

void scheduler_start_up(scheme_t s) {
    switch (s) {
    case FCFS:
        comparision_func = comparer_fcfs;
        break;
    case PRI:
        comparision_func = comparer_pri;
        break;
    case PPRI:
        comparision_func = comparer_ppri;
        break;
    case PSRTF:
        comparision_func = comparer_psrtf;
        break;
    case RR:
        comparision_func = comparer_rr;
        break;
    case SJF:
        comparision_func = comparer_sjf;
        break;
    default:
        printf("Did not recognize scheme\n");
        exit(1);
    }
    priqueue_init(&pqueue, comparision_func);
    pqueue_scheme = s;
    // Put any additional set up code you may need here
}

static int break_tie(const void *a, const void *b) {
    return comparer_fcfs(a, b);
}

int comparer_fcfs(const void *a, const void *b) {
    // TODO: Implement me!
    job_info* a_info = ((job*)a)->metadata;
    job_info* b_info = ((job*)b)->metadata;
    return a_info->arrival_time < b_info->arrival_time ? -1 : 1;
}

int comparer_ppri(const void *a, const void *b) {
    // Complete as is
    return comparer_pri(a, b);
}

int comparer_pri(const void *a, const void *b) {
    // TODO: Implement me!
    job_info* a_info = ((job*)a)->metadata;
    job_info* b_info = ((job*)b)->metadata;
    int cmp = a_info->priority - b_info->priority;
    if (cmp < 0) {
        return -1;
    } else if (cmp > 0) {
        return 1;
    }
    return break_tie(a, b);
}

int comparer_psrtf(const void *a, const void *b) {
    // TODO: Implement me!
    job_info* a_info = ((job*)a)->metadata;
    job_info* b_info = ((job*)b)->metadata;
    //Shortest Remaining Time First
    int cmp = a_info->remaining_time - b_info->remaining_time;
    if (cmp < 0) {
        return -1;
    } else if (cmp > 0) {
        return 1;
    }
    return break_tie(a, b);
}

int comparer_rr(const void *a, const void *b) {
    // TODO: Implement me!
    job_info* a_info = ((job*)a)->metadata;
    job_info* b_info = ((job*)b)->metadata;
    int cmp = a_info->rr_time - b_info->rr_time;
    if (cmp < 0) {
        return -1;
    } else if (cmp > 0) {
        return 1;
    }
    return break_tie(a, b);
}

int comparer_sjf(const void *a, const void *b) {
    // TODO: Implement me!
    job_info* a_info = ((job*)a)->metadata;
    job_info* b_info = ((job*)b)->metadata;
    //Shortest Job First
    int cmp = a_info->required_run_time - b_info->required_run_time;
    if (cmp < 0) {
        return -1;
    } else if (cmp > 0) {
        return 1;
    }
    return break_tie(a, b);
}

// Do not allocate stack space or initialize ctx. These will be overwritten by
// gtgo
void scheduler_new_job(job *newjob, int job_number, double time,
                       scheduler_info *sched_data) {
    // TODO: Implement me!
    job_info* info = calloc(1, sizeof(job_info));
    info->id = job_number;
    info->arrival_time = time;
    info->required_run_time = sched_data->running_time;
    info->remaining_time = sched_data->running_time;
    info->priority = sched_data->priority;
    info->rr_time = -1;
    info->start_time = -1;
    newjob->metadata = info;
    priqueue_offer(&pqueue, newjob);
}

job *scheduler_quantum_expired(job *job_evicted, double time) {
    // TODO: Implement me!
    if (!job_evicted) {
        return priqueue_poll(&pqueue);
    }
    job_info* info = job_evicted->metadata;
    info->remaining_time -= 1;
    info->rr_time = time;
    if (info->start_time == -1) {
        //set start time
        info->start_time = time - 1;
    }
    if (pqueue_scheme == PPRI || pqueue_scheme == PSRTF || pqueue_scheme == RR) {
        //the current scheme is preemptive and job_evicted is not NULL, 
        //place job_evicted back on the queue and return the next job that should be ran.
        priqueue_offer(&pqueue, job_evicted);
        return priqueue_poll(&pqueue);
    }
    //the current scheme is non-preemptive and job_evicted is not NULL, return job_evicted.
    return job_evicted;
}

void scheduler_job_finished(job *job_done, double time) {
    // TODO: Implement me!
    job_info* info = job_done->metadata;
    total_waiting_time += time - info->arrival_time - info->required_run_time;
    total_response_time += info->start_time - info->arrival_time;
    total_turnaround_time += time - info->arrival_time;
    num_jobs++;
    free(info);
}

static void print_stats() {
    fprintf(stderr, "turnaround     %f\n", scheduler_average_turnaround_time());
    fprintf(stderr, "total_waiting  %f\n", scheduler_average_waiting_time());
    fprintf(stderr, "total_response %f\n", scheduler_average_response_time());
}

double scheduler_average_waiting_time() {
    // TODO: Implement me!
    return total_waiting_time / num_jobs;
}

double scheduler_average_turnaround_time() {
    // TODO: Implement me!
    return total_turnaround_time / num_jobs;
}

double scheduler_average_response_time() {
    // TODO: Implement me!
    return total_response_time / num_jobs;
}

void scheduler_show_queue() {
    // OPTIONAL: Implement this if you need it!
}

void scheduler_clean_up() {
    priqueue_destroy(&pqueue);
    print_stats();
}
