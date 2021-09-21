/**
 * utilities_unleashed
 * CS 241 - Spring 2021
 */
#include "format.h"
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_time_usage();
    }

    double duration = 0;
    struct timespec tp_start, tp_end;
    clockid_t clk_id = CLOCK_MONOTONIC;
    clock_gettime(clk_id, &tp_start);
    
    pid_t child = fork();
    if (child == -1) {
        print_fork_failed();
    }
    if (child == 0) {
        //is child
        execvp(argv[1], argv + 1);
        print_exec_failed();
    } else {
        //is parent
        int status = 0;
        pid_t w = waitpid(child, &status, 0);
        if (w == -1) {
            exit(1);
        }
        clock_gettime(clk_id, &tp_end);
        if (WIFEXITED(status)) {
            duration = (tp_end.tv_sec - tp_start.tv_sec) + (tp_end.tv_nsec - tp_start.tv_nsec) / 1e9;
            display_results(argv, duration);
        }
    }
    return 0;
}
