/**
 * vector
 * CS 241 - Spring 2021
 */
#include "sstring.h"
#include "vector.h"

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <assert.h>
#include <string.h>

struct sstring {
    // Anything you want
    char* string;
    size_t length;
};

sstring *cstr_to_sstring(const char *input) {
    // your code goes here
    sstring* s = malloc(sizeof(sstring));
    s->string = calloc(strlen(input) + 1, sizeof(char));
    s->length = strlen(input);
    strcpy(s->string, input);
    return s;
}

char *sstring_to_cstr(sstring *input) {
    // your code goes here
    char* res = calloc(input->length + 1, sizeof(char));
    strcpy(res, input->string);
    return res;
}

int sstring_append(sstring *this, sstring *addition) {
    // your code goes here
    this->length += addition->length;
    this->string = realloc(this->string, this->length + 1);
    strcat(this->string, addition->string);
    return this->length;
}

vector *sstring_split(sstring *this, char delimiter) {
    // your code goes here
    vector* res = string_vector_create();
    /*
    char* cur = strtok(this->string, &delimiter);
    while (cur) {
        vector_push_back(res, cur);
        cur = strtok(NULL, &delimiter);
    }
    */
    size_t l = 0;
    size_t r = 0;
    while (l < this->length) {
        while (r < this->length && this->string[r] != delimiter) r++;
        char* cur = calloc(r - l + 1, 1);
        size_t i = 0;
        for (; i < r - l; i++) {
            cur[i] = this->string[l + i];
        }
        vector_push_back(res, cur);
        free(cur);
        if (r == this->length - 1) {
            char* last = calloc(1, 1);
            vector_push_back(res, last);
            free(last);
        }
        r++;
        l = r;
    }
    return res;
}

int str_compare(char* str, char *target) {
    for (size_t i = 0; i < strlen(target); i++) {
        if (str[i] == 0) return -1;
        if (str[i] != target[i]) return 0;
    }
    return 1;
}

int sstring_substitute(sstring *this, size_t offset, char *target,
                       char *substitution) {
    // your code goes here
    size_t i = offset;
    for (; i < this->length; i++) {
        int flag = str_compare(this->string + i, target);
        if (flag == 1) {
            size_t t_len = strlen(target);
            size_t s_len = strlen(substitution);
            size_t j = 0;
            if (t_len < s_len) {
                //target less than the substitution
                this->length = this->length - t_len + s_len;
                this->string = realloc(this->string, this->length + 1);
                
                for (j = this->length; j >= i + s_len; j--) {
                    this->string[j] = this->string[j - s_len + t_len];
                }
            } else if (t_len > s_len) {
                //target longer than the substitution
                this->length = this->length - t_len + s_len;
                
                for (j = i + s_len; j <= this->length; j++) {
                    this->string[j] = this->string[j - s_len + t_len];
                }
                this->string = realloc(this->string, this->length + 1);
            }
            //substitute the substring
            for (j = 0; j < s_len; j++) {
                this->string[i + j] = substitution[j];
            }
            
            return 0;
        }
        if (flag == -1) return -1;
    }
    return -1;
}

char *sstring_slice(sstring *this, int start, int end) {
    // your code goes here
    char* res = calloc(end - start + 1, 1);
    int i = 0;
    for (; i < end - start; i++) {
        res[i] = this->string[start + i];
    }
    return res;
}

void sstring_destroy(sstring *this) {
    // your code goes here
    free(this->string);
    free(this);
    this = NULL;
}
