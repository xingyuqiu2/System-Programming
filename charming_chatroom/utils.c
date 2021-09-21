/**
 * charming_chatroom
 * CS 241 - Spring 2021
 */
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "utils.h"
static const size_t MESSAGE_SIZE_DIGITS = 4;

char *create_message(char *name, char *message) {
    int name_len = strlen(name);
    int msg_len = strlen(message);
    char *msg = calloc(1, msg_len + name_len + 4);
    sprintf(msg, "%s: %s", name, message);

    return msg;
}

ssize_t get_message_size(int socket) {
    int32_t size;
    ssize_t read_bytes =
        read_all_from_socket(socket, (char *)&size, MESSAGE_SIZE_DIGITS);
    if (read_bytes == 0 || read_bytes == -1)
        return read_bytes;

    return (ssize_t)ntohl(size);
}

// You may assume size won't be larger than a 4 byte integer
ssize_t write_message_size(size_t size, int socket) {
    // Your code here
    ssize_t buffer_size = htonl(size);
    return write_all_to_socket(socket, (char*)&buffer_size, MESSAGE_SIZE_DIGITS);
}

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
