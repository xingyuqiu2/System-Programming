/**
 * nonstop_networking
 * CS 241 - Spring 2021
 */
#include "common.h"
#include <unistd.h>
#include <errno.h>
#include <stdio.h>

ssize_t read_all_from_socket(int socket, char *buffer, size_t count) {
    // Your Code Here
    size_t total_amount_read = 0;
    while (total_amount_read < count) {
        ssize_t bytes_read = read(socket, buffer + total_amount_read, count - total_amount_read);
        if (bytes_read == 0) {
            break;
        } else if (bytes_read > 0) {
            total_amount_read += bytes_read;
        } else if (bytes_read == -1 && errno == EINTR) {
            continue;
        } else {
            return -1;
        }
    }
    return total_amount_read;
}

ssize_t write_all_to_socket(int socket, const char *buffer, size_t count) {
    // Your Code Here
    size_t total_amount_written = 0;
    while (total_amount_written < count) {
        ssize_t bytes_written = write(socket, buffer + total_amount_written, count - total_amount_written);
        if (bytes_written == 0) {
            break;
        } else if (bytes_written > 0) {
            total_amount_written += bytes_written;
        } else if (bytes_written == -1 && errno == EINTR) {
            continue;
        } else {
            return -1;
        }
    }
    return total_amount_written;
}

int write_localfile_to_socket(int socket, FILE* local_file, size_t file_size) {
    size_t total_bytes_written = 0;
    size_t blocksize = 512;
    char buf[blocksize];
    //write blocksize of bytes each time
    while (total_bytes_written < file_size) {
        if (file_size - total_bytes_written < blocksize) {
            blocksize = file_size - total_bytes_written;
        }
        size_t bytes_read = fread(buf, sizeof(char), blocksize, local_file);
        if (bytes_read == 0) break;
        if (write_all_to_socket(socket, buf, bytes_read) == -1) {
            return -1;
        }
        total_bytes_written += bytes_read;
    }
    return 0;
}