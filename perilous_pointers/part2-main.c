/**
 * perilous_pointers
 * CS 241 - Spring 2021
 */
#include "part2-functions.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * (Edit this function to print out the "Illinois" lines in
 * part2-functions.c in order.)
 */
int main() {
    // your code here
    first_step(81);

    int* second_value = malloc(sizeof(int));
    *second_value = 132;
    second_step(second_value);
    free(second_value);
    second_value = NULL;

    int** third_value = calloc(2, sizeof(int*));
    *third_value = calloc(2, sizeof(int));
    *third_value[0] = 8942;
    double_step(third_value);
    free(third_value[0]);
    free(third_value);
    third_value = NULL;

    char* fourth_value = calloc(21, sizeof(char));
    *(int *)(fourth_value + 5) = 15;
    strange_step(fourth_value);
    free(fourth_value);
    fourth_value = NULL;

    void* fifth_value = calloc(5, sizeof(char));
    ((char *)fifth_value)[3] = 0;
    empty_step(fifth_value);
    free(fifth_value);
    fifth_value = NULL;

    char* s2 = calloc(5, sizeof(char));
    s2[3] = 'u';
    void* s = s2;
    two_step(s, s2);
    free(s2);
    s2 = NULL;
    s = NULL;

    char *first = calloc(1, sizeof(char));
    char *second = first + 2;
    char *third = second + 2;
    three_step(first, second, third);
    free(first);
    first = NULL;
    second = NULL;
    third = NULL;

    first = calloc(3, sizeof(char));
    second = calloc(4, sizeof(char));
    third = calloc(5, sizeof(char));
    second[2] = first[1] + 8;
    third[3] = second[2] + 8;
    step_step_step(first, second, third);
    free(first);
    first = NULL;
    free(second);
    second = NULL;
    free(third);
    third = NULL;

    char a = 1;
    int b = 1;
    it_may_be_odd(&a, b);

    char str[20] = ",next,CS241";
    tok_step(str);

    void *blue = calloc(5, sizeof(char));
    ((char*) blue)[0] = 1;  //2^1 = 1
    ((char*) blue)[1] = 2;  //2^9 = 51
    the_end(blue, blue);
    free(blue);
    return 0;
}
