/**
 * mad_mad_access_patterns
 * CS 241 - Spring 2021
 */
#include "tree.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static FILE* file;
/*
  Look up a few nodes in the tree and print the info they contain.
  This version uses fseek() and fread() to access the data.

  ./lookup1 <data_file> <word> [<word> ...]
*/
void binarysearch(char* word, long offset) {
    if (offset == 0) {
        printNotFound(word);
        return;
    }
    //set the position to the start of the BinaryTreeNode
    fseek(file, offset, SEEK_SET);
    BinaryTreeNode* cur = calloc(1, sizeof(BinaryTreeNode));
    //read the BinaryTreeNode into cur
    if (fread(cur, 1, sizeof(BinaryTreeNode), file) != sizeof(BinaryTreeNode)) {
        //error if this occur
        printNotFound(word);
        free(cur);
        return;
    }
    char cur_word[31];
    memset(cur_word, 0, 31);
    //set the position to the start of the current word
    fseek(file, offset + sizeof(BinaryTreeNode), SEEK_SET);
    fread(cur_word, 1, 30, file);
    //if word match, found
    if (strcmp(word, cur_word) == 0) {
        printFound(word, cur->count, cur->price);
        free(cur);
        return;
    }
    //search left or right
    if (strcmp(word, cur_word) < 0) {
        binarysearch(word, cur->left_child);
    } else {
        binarysearch(word, cur->right_child);
    }
    free(cur);
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
    file = fopen(data_file, "r");
    if (!file) {
        openFail(data_file);
        exit(2);
    }
    char buf[BINTREE_ROOT_NODE_OFFSET + 1];
    fread(buf, sizeof(char), BINTREE_ROOT_NODE_OFFSET, file);
    buf[BINTREE_ROOT_NODE_OFFSET] = 0;
    if (strcmp(BINTREE_HEADER_STRING, buf)) {
        formatFail(data_file);
        fclose(file);
        exit(2);
    }

    for (int i = 2; i < argc; i++) {
        binarysearch(argv[i], BINTREE_ROOT_NODE_OFFSET);
    }
    fclose(file);
    return 0;
}
