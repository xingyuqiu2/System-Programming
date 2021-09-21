/**
 * mad_mad_access_patterns
 * CS 241 - Spring 2021
 */
#include "tree.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

static char* file;

/*
  Look up a few nodes in the tree and print the info they contain.
  This version uses mmap to access the data.

  ./lookup2 <data_file> <word> [<word> ...]
*/
void binarysearch(char* word, long offset) {
    if (offset == 0) {
        printNotFound(word);
        return;
    }
    //get the current BinaryTreeNode
    BinaryTreeNode* cur = (BinaryTreeNode*)(file + offset);
    char cur_word[31];
    memset(cur_word, 0, 31);
    //copy the current word
    strncpy(cur_word, (char*)(cur + 1), 30);
    //if word match, found
    if (strcmp(word, cur_word) == 0) {
        printFound(word, cur->count, cur->price);
        return;
    }
    //search left or right
    if (strcmp(word, cur_word) < 0) {
        binarysearch(word, cur->left_child);
    } else {
        binarysearch(word, cur->right_child);
    }
}

//Error cases:
//If run with less than 2 arguments, your program should print an error message describing the arguments it expects and exit with error code 1.
//If the data file cannot be read or the first 4 bytes are not “BTRE”, print a helpful error message and exit with error code 2.
int main(int argc, char **argv) {
    if (argc < 3) {
        printArgumentUsage();
        exit(1);
    }
    char* data_file = argv[1];
    int fd = open(data_file, O_RDONLY);
    if (fd == -1) {
        openFail(data_file);
        exit(2);
    }
    struct stat s;
    if (stat(data_file, &s) == -1) {
        openFail(data_file);
        close(fd);
        exit(2);
    }
    
    file = mmap(NULL, s.st_size, PROT_READ, MAP_SHARED, fd, 0);
    if (file == (void *) -1) {
        mmapFail(data_file);
        close(fd);
        exit(2);
    }

    char head[5];
    memset(head, 0, 5);
    strncpy(head, file, 4);
    if (strcmp(BINTREE_HEADER_STRING, head)) {
        formatFail(data_file);
        close(fd);
        exit(2);
    }

    for (int i = 2; i < argc; i++) {
        binarysearch(argv[i], BINTREE_ROOT_NODE_OFFSET);
    }

    close(fd);
    return 0;
}
