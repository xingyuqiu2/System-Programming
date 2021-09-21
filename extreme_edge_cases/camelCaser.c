/**
 * extreme_edge_cases
 * CS 241 - Spring 2021
 */
#include "camelCaser.h"
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h> 

char **camel_caser(const char *input_str) {
    // TODO: Implement me!
    if (input_str == NULL) return NULL;
    int inputIdx = 0;   //iterator index of input_str
    char cur = 0;   //current char
    int numberOfSentence = 0;
    while ((cur = input_str[inputIdx++])) {
        if (ispunct(cur)) {
            numberOfSentence++;
        }
    }
    int lenOfArray = numberOfSentence;  //store the length of the output array(same as the number of sentences except NULL)
    inputIdx = 0;
    char** res = (char**) malloc((numberOfSentence + 1) * sizeof(char*));
    //initialize pointers in res
    int i = 0;
    for (i = 0; i < lenOfArray + 1; i++) {
        res[i] = NULL;
    }

    numberOfSentence = 0;
    int numberOfChar = 0;
    while ((cur = input_str[inputIdx++])) {
        if (numberOfSentence == lenOfArray) {
            //after the end of the last sentence
            res[numberOfSentence] = NULL;
            break;
        }
        if (ispunct(cur)) {
            res[numberOfSentence] = (char*) malloc((numberOfChar + 1) * sizeof(char));
            //initialize chars in res
            for (i = 0; i < numberOfChar + 1; i++) {
                res[numberOfSentence][i] = 0;
            }
            numberOfSentence++;
            numberOfChar = 0;
        } else if (!isspace(cur)) {
            //if cur is a letter or other characters
            numberOfChar++;
        }
    }

    inputIdx = 0;
    numberOfSentence = 0;
    numberOfChar = 0;
    bool capitalFlag = false;
    while ((cur = input_str[inputIdx++])) {
        if (numberOfSentence == lenOfArray) {
            //after the end of the last sentence
            break;
        }
        if (ispunct(cur)) {
            res[numberOfSentence][numberOfChar] = '\0';    //set the last char of every sentence to 0
            capitalFlag = false;
            numberOfSentence++;
            numberOfChar = 0;
        } else if (isspace(cur)) {
            //meet the space and next word is not the first word in the sentence
            if (numberOfChar != 0) capitalFlag = true;
        } else if (!isalpha(cur)) {
            //is non of the specified chars, keep the same
            res[numberOfSentence][numberOfChar] = cur;
            numberOfChar++;
        } else {
            //is a letter
            if (capitalFlag) {
                res[numberOfSentence][numberOfChar] = toupper(cur);
                capitalFlag = false;
            } else {
                res[numberOfSentence][numberOfChar] = tolower(cur);
            }
            numberOfChar++;
        }
    }
    return res;
}

void destroy(char **result) {
    // TODO: Implement me!
    if (result == NULL) return;
    int idx = 0;
    while (result[idx]) {
        //while current string is not NULL
        free(result[idx]);
        result[idx] = NULL;
        idx++;
    }
    free(result);
    return;
}
