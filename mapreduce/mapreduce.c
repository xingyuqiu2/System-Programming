/**
 * mapreduce
 * CS 241 - Spring 2021
 */
#include "utils.h"
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>

int main(int argc, char **argv) {
    if (argc != 6) exit(1);
    char* input_file = argv[1];
    char* output_file = argv[2];
    char* mapper_executable = argv[3];
    char* reducer_executable = argv[4];
    char* count = argv[5];
    int mapper_count = atoi(count);

    // Create an input pipe for each mapper.
    int mapper_pipe[mapper_count * 2];
    for (int i = 0; i < mapper_count * 2; i += 2) {
        pipe(mapper_pipe + i);
    }

    // Create one input pipe for the reducer.
    int reducer_pipe[2];
    pipe(reducer_pipe);

    // Open the output file.
    int output_file_dp = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, S_IWUSR | S_IRUSR);

    // Start a splitter process for each mapper.
    int splitter_pid[mapper_count];
    for (int i = 0; i < mapper_count; i++) {
        splitter_pid[i] = fork();
        if (splitter_pid[i] == -1) exit(1);
        if (splitter_pid[i] == 0) {
            //child
            char string_i[10];
            sprintf(string_i, "%d", i);
            close(mapper_pipe[i * 2]);
            if (dup2(mapper_pipe[i * 2 + 1], 1) == -1) exit(1);
            execl("splitter", "splitter", input_file, count, string_i, NULL);
            exit(1);
        }
    }

    // Start all the mapper processes.
    int mapper_pid[mapper_count];
    for (int i = 0; i < mapper_count; i++) {
        close(mapper_pipe[i * 2 + 1]);
        mapper_pid[i] = fork();
        if (mapper_pid[i] == -1) exit(1);
        if (mapper_pid[i] == 0) {
            //child
            close(reducer_pipe[0]);
            if (dup2(mapper_pipe[i * 2], 0) == -1) exit(1);
            if (dup2(reducer_pipe[1], 1) == -1) exit(1);
            execl(mapper_executable, mapper_executable, NULL);
            exit(1);
        }
    }

    // Start the reducer process.
    int reducer_pid = fork();
    if (reducer_pid == -1) exit(1);
    if (reducer_pid == 0) {
        //child
        close(reducer_pipe[1]);
        if (dup2(reducer_pipe[0], 0) == -1) exit(1);
        if (dup2(output_file_dp, 1) == -1) exit(1);
        execl(reducer_executable, reducer_executable, NULL);
        exit(1);
    }
    //close all pipes
    for (int i = 0; i < mapper_count; i++) {
        close(mapper_pipe[i]);
    }
    close(reducer_pipe[0]);
    close(reducer_pipe[1]);

    // Wait for the reducer to finish.
    // Print nonzero subprocess exit codes.
    for (int i = 0; i < mapper_count; i++) {
        int status;
        waitpid(splitter_pid[i], &status, 0);
        if (WIFEXITED(status)) {
            print_nonzero_exit_status("splitter", WEXITSTATUS(status));
        }
    }
    for (int i = 0; i < mapper_count; i++) {
        int status;
        waitpid(mapper_pid[i], &status, 0);
        if (WIFEXITED(status)) {
            print_nonzero_exit_status(mapper_executable, WEXITSTATUS(status));
        }
    }
    int status;
    waitpid(reducer_pid, &status, 0);
    if (WIFEXITED(status)) {
        print_nonzero_exit_status(reducer_executable, WEXITSTATUS(status));
    }

    // Count the number of lines in the output file.
    print_num_lines(output_file);
    close(output_file_dp);
    return 0;
}
