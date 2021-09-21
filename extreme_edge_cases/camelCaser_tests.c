/**
 * extreme_edge_cases
 * CS 241 - Spring 2021
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "camelCaser.h"
#include "camelCaser_tests.h"

//return the length of the array
int arrayLen(char** str) {
    //if str is NULL, return -1
    if (!str) return -1;
    
    char* cur = NULL;
    int idx = 0;
    int len = 0;
    while ((cur = str[idx++])) {
        len++;
    }
    return len;
}

//helper function to test camelCaser
int test(char* input, char** answer, char **(*camelCaser)(const char *), void (*destroy)(char **)) {
    char** output_str = camelCaser(input);
    int len1 = arrayLen(output_str);
    int len2 = arrayLen(answer);

    //printf("length of output: %d\n", len1);
    //printf("length of answer: %d\n", len2);

    if (len1 != len2) {
        destroy(output_str);
        return 0;
    }
    
    int i = 0;
    for (; i < len1; i++) {
        //printf("output sentence: %s\n", output_str[i]);
        //printf("answer sentence: %s\n", answer[i]);

        if (strcmp(output_str[i], answer[i]) != 0) {
            //string has difference
            destroy(output_str);
            return 0;
        }
    }
    destroy(output_str);
    return 1;
}

int test_camelCaser(char **(*camelCaser)(const char *),
                    void (*destroy)(char **)) {
    // TODO: Implement me!
    char* input1 = "The Heisenbug is an incredible creature. Facenovel servers get their power from its indeterminism. Code smell can be ignored with INCREDIBLE use of air freshener. God objects are the new religion.";
    char* answer1[] = {"theHeisenbugIsAnIncredibleCreature",
        "facenovelServersGetTheirPowerFromItsIndeterminism",
        "codeSmellCanBeIgnoredWithIncredibleUseOfAirFreshener",
        "godObjectsAreTheNewReligion",
        NULL};

    char* input2 = "";
    char* answer2[] = {NULL};

    char* input3 = "try m/any p{unct )chars, this  sh=Ould wo+rk. ";
    char* answer3[] = {"tryM",
        "anyP",
        "unct",
        "chars",
        "thisSh",
        "ouldWo",
        "rk",
        NULL};

    char* input4 = "Lets TRY cPitallize  chArs. TO seE iF IT wORks.  ";
    char* answer4[] = {"letsTryCpitallizeChars",
        "toSeeIfItWorks",
        NULL};

    char* input5 = "to see4 if seten5ce 404Can ; contain 7other 666CHars.";
    char* answer5[] = {"toSee4IfSeten5ce404Can",
        "contain7Other666Chars",
        NULL};

    char* input6 = ".to see4 what if < ends with se4veral punct ; like this. :   ;<  ";
    char* answer6[] = {"",
        "toSee4WhatIf",
        "endsWithSe4veralPunct",
        "likeThis",
        "",
        "",
        "",
        NULL};

    char* input7 = NULL;
    char** answer7 = NULL;

    char* input8 = " should not include this sentence ";
    char* answer8[] = {NULL};

    char* input9 = " if CORRECT \0 should not include this sentence . ";
    char* answer9[] = {NULL};

    char* input10 = "  trY \n new .lIne . ";
    char* answer10[] = {"tryNew",
        "line",
        NULL};


    if (!test(input1, answer1, camelCaser, destroy)) {
        fprintf(stderr, "test1 failed\n");
        return 0;
    }
    
    if (!test(input2, answer2, camelCaser, destroy)) {
        fprintf(stderr, "test2 failed\n");
        return 0;
    }
    
    if (!test(input3, answer3, camelCaser, destroy)) {
        fprintf(stderr, "test3 failed\n");
        return 0;
    }
    
    if (!test(input4, answer4, camelCaser, destroy)) {
        fprintf(stderr, "test4 failed\n");
        return 0;
    }
    
    if (!test(input5, answer5, camelCaser, destroy)) {
        fprintf(stderr, "test5 failed\n");
        return 0;
    }

    if (!test(input6, answer6, camelCaser, destroy)) {
        fprintf(stderr, "test6 failed\n");
        return 0;
    }

    if (!test(input7, answer7, camelCaser, destroy)) {
        fprintf(stderr, "test7 failed\n");
        return 0;
    }

    if (!test(input8, answer8, camelCaser, destroy)) {
        fprintf(stderr, "test8 failed\n");
        return 0;
    }

    if (!test(input9, answer9, camelCaser, destroy)) {
        fprintf(stderr, "test9 failed\n");
        return 0;
    }

    if (!test(input10, answer10, camelCaser, destroy)) {
        fprintf(stderr, "test10 failed\n");
        return 0;
    }

    return 1;
}
