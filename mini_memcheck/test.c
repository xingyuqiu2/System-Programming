/**
 * mini_memcheck
 * CS 241 - Spring 2021
 */
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    // Your tests here using malloc and free
    void *p1 = malloc(30);
    void *p2 = malloc(40);
    void *p3 = malloc(50);
    free(p2);
    free(p1);
    free(p3);
    free(p3);
    return 0;
}