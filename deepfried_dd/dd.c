/**
 * deepfried_dd
 * CS 241 - Spring 2021
 */
#include "format.h"
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <signal.h>

static struct timespec tp_start, tp_end;
static size_t full_blocks_in;
static size_t partial_blocks_in;
static size_t full_blocks_out;
static size_t partial_blocks_out;
static size_t total_bytes_copied;


void live_status_report(int signal) {
    if (signal == SIGUSR1) {
        clock_gettime(CLOCK_MONOTONIC, &tp_end);
        double duration = (tp_end.tv_sec - tp_start.tv_sec) + (tp_end.tv_nsec - tp_start.tv_nsec) / 1e9;
        print_status_report(full_blocks_in, partial_blocks_in, full_blocks_out, partial_blocks_out, total_bytes_copied, duration);
        fflush(stdout);
    }
}

int main(int argc, char **argv) {
    signal(SIGUSR1, live_status_report);
    char *inputfile = NULL, *outputfile = NULL;
    size_t blocksize = 512, total_num_blocks_copied = -1, num_blocks_skip_at_input = 0, num_blocks_skip_at_output = 0;
    //get start time
    double duration = 0;
    clock_gettime(CLOCK_MONOTONIC, &tp_start);
    //parse argv
    int opt;
    if (argc >= 2) {
        while ((opt = getopt(argc, argv, "i:o:b:c:p:k:")) != -1) {
            switch (opt) {
            case 'i':
                if (optarg) {
                    inputfile = calloc(1, strlen(optarg) + 1);
                    strcpy(inputfile, optarg);
                }
                break;
            case 'o':
                if (optarg) {
                    outputfile = calloc(1, strlen(optarg) + 1);
                    strcpy(outputfile, optarg);
                }
                break;
            case 'b':
                if (optarg) {
                    blocksize = atoi(optarg);
                }
                break;
            case 'c':
                if (optarg) {
                    total_num_blocks_copied = atoi(optarg);
                }
                break;
            case 'p':
                if (optarg) {
                    num_blocks_skip_at_input = atoi(optarg);
                }
                break;
            case 'k':
                if (optarg) {
                    num_blocks_skip_at_output = atoi(optarg);
                }
                break;
            default: /* '?' */
                exit(1);
            }
        }
    }
    FILE* input = stdin;
    FILE* output = stdout;

    //get the input fd and output fd
    if (inputfile) {
        input = fopen(inputfile, "r");
        if (!input) {
            print_invalid_input(inputfile);
            free(inputfile);
            exit(1);
        }
    }
    if (outputfile) {
        output = fopen(outputfile, "w+");
        if (!output) {
            print_invalid_output(outputfile);
            free(outputfile);
            exit(1);
        }
    }

    //fseek to the correct position
    if (input != stdin) {
        long offset = num_blocks_skip_at_input * blocksize;
        fseek(input, offset, SEEK_SET);
    }
    if (output != stdout) {
        long offset = num_blocks_skip_at_output * blocksize;
        fseek(output, offset, SEEK_SET);
    }

    //read from input and write to output
    char buffer[blocksize];
    while (!feof(input)) {
        size_t bytes_read = fread(buffer, sizeof(char), blocksize, input);
        if (bytes_read == blocksize) {
            //full block
            full_blocks_in++;
            size_t bytes_written = fwrite(buffer, sizeof(char), bytes_read, output);
            fflush(output);
            total_bytes_copied += bytes_written;
            full_blocks_out++;
        } else {
            if (bytes_read == 0) break;
            //partial block
            partial_blocks_in++;
            size_t bytes_written = fwrite(buffer, sizeof(char), bytes_read, output);
            fflush(output);
            total_bytes_copied += bytes_written;
            partial_blocks_out++;
        }
        if (full_blocks_out + partial_blocks_out == total_num_blocks_copied) break;
    }

    //get end time
    clock_gettime(CLOCK_MONOTONIC, &tp_end);
    duration = (tp_end.tv_sec - tp_start.tv_sec) + (tp_end.tv_nsec - tp_start.tv_nsec) / 1e9;
    //print
    print_status_report(full_blocks_in, partial_blocks_in, full_blocks_out, partial_blocks_out, total_bytes_copied, duration);
    fflush(stdout);
    //free memory
    if (inputfile) free(inputfile);
    if (outputfile) free(outputfile);
    if (input != stdin) fclose(input);
    if (output != stdout) fclose(output);
    return 0;
}