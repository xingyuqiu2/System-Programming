/**
 * vector
 * CS 241 - Spring 2021
 */
#include "vector.h"
#include <stdio.h>
#include <string.h>

//helper function to print sth.
void p_size_capacity(vector* v) {
    printf("vector size: %zu    ", vector_size(v));
    printf("capacity: %zu\n", vector_capacity(v));
}

void p_elements(vector* v) {
    printf("[");
    size_t i = 0;
    if (i < vector_size(v)) printf("%s", vector_get(v, i));
    for (i = 1; i < vector_size(v); i++) {
        printf(",%s", vector_get(v, i));
    }
    printf("]\n");
}

int main(int argc, char *argv[]) {
    // Write your test cases here
    vector* v1 = string_vector_create();

    //test vector_push_back
    for (int i = 0; i < 15; i++) {
        vector_push_back(v1, "ab");
        p_elements(v1);
        p_size_capacity(v1);
    }

    //test vector_back
    vector_push_back(v1, "end");
    printf("%s\n", *vector_back(v1));
    p_elements(v1);
    p_size_capacity(v1);

    //test vector_insert
    vector_insert(v1, 9, "9");
    p_elements(v1);
    p_size_capacity(v1);

    //test vector_set
    vector_set(v1, 10, "10");
    p_elements(v1);
    p_size_capacity(v1);

    //test vector_reserve
    vector_reserve(v1, 33);
    p_elements(v1);
    p_size_capacity(v1);

    //test vector_erase
    for (int i = 0; i < 5; i++) {
        vector_erase(v1, 5);
        p_elements(v1);
        p_size_capacity(v1);
    }

    //test vector_pop_back
    vector_pop_back(v1);
    p_elements(v1);
    p_size_capacity(v1);

    //test vector_insert
    vector_insert(v1, 3, "3");
    p_elements(v1);
    p_size_capacity(v1);

    //test vector_resize
    vector_resize(v1, 5);
    p_elements(v1);
    p_size_capacity(v1);

    //resize with 0
    vector_resize(v1, 0);
    p_elements(v1);
    p_size_capacity(v1);

    //test vector_insert
    vector_insert(v1, 0, "insert1at(0)");
    p_elements(v1);
    p_size_capacity(v1);
    vector_insert(v1, 0, "insert2at(0)");
    p_elements(v1);
    p_size_capacity(v1);
    vector_insert(v1, 2, "insert3at(2)");
    p_elements(v1);
    p_size_capacity(v1);

    //test vector_destroy
    vector_destroy(v1);

    return 0;
}
