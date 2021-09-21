/**
 * finding_filesystems
 * CS 241 - Spring 2021
 */
#include "minixfs.h"
#include "minixfs_utils.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

int main(int argc, char *argv[]) {
    // Write tests here!
    file_system *fs = open_fs("test.fs");
    if (!fs) {
        return -1;
    }
    off_t off = 0;
    char expected[14];
    memset(expected, 0, 14);
    ssize_t bytes_read = minixfs_read(fs, "/goodies/hello.txt", expected, 13, &off);
    if (bytes_read == -1) {
        printf("fail 1\n");
        close_fs(&fs);
        return -1;
    }
    off = 0;
    ssize_t bytes_written = minixfs_write(fs, "/newhello.txt", expected, 13, &off);
    if (bytes_written == -1) {
        printf("fail 2\n");
        close_fs(&fs);
        return -1;
    }
    off = 0;
    char buf[14];
    memset(buf, 0, 14);
    ssize_t bytes_read_out = minixfs_read(fs, "/newhello.txt", buf, 13, &off);
    if (bytes_read_out == -1) {
        printf("fail 3\n");
        close_fs(&fs);
        return -1;
    }
    //open /goodies/hello.txt with open() or fopen() and read the contents way you normally would
    if (!strcmp(buf, expected)) {
        printf("same\n");
    } else {
        printf("different\n");
        printf("expected: %s\n", expected);
        printf("buf: %s\n", buf);
    }
    close_fs(&fs);
    return 0;
}
