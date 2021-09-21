/**
 * utilities_unleashed
 * CS 241 - Spring 2021
 */
#include "format.h"
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

void parse_argument(char* input, size_t len_str) {
    char *key = calloc(len_str + 1, 1);
    char *val = calloc(len_str + 1, 1);
    size_t j = 0, len_key = 0, len_val = 0;
    int iskey = 1, findEqual = 0, needGet = 0;
    for (; j < len_str; j++) {
        if (input[j] == '=') {
            //find = in argument
            findEqual = 1;
            iskey = 0;
            continue;
        }

        if (iskey) {
            //is key part
            key[len_key++] = input[j];
        } else {
            //is value part
            if (input[j] == '%') {
                if (len_val == 0) {
                    //need to set %val to key
                    needGet = 1;
                    continue;
                } else {
                    //invalid argument
                    free(key);
                    free(val);
                    print_environment_change_failed();
                }
            }
            val[len_val++] = input[j];
        }
    }
    if (!findEqual) {
        //Cannot find = in an variable argument
        free(key);
        free(val);
        print_env_usage();
    }
    if (needGet) {
        char* new_val = getenv(val);
        if (new_val == NULL) {
            free(key);
            free(val);
            print_environment_change_failed();
        }
        int res = setenv(key, new_val, 0);
        free(key);
        free(val);
        if (res == -1) {
            print_environment_change_failed();
        }
    } else {
        int res = setenv(key, val, 0);
        free(key);
        free(val);
        if (res == -1) {
            print_environment_change_failed();
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        print_env_usage();
    }

    pid_t child = fork();
    if (child == -1) {
        print_fork_failed();
    }
    if (child == 0) {
        //is child
        int i = 1;
        for (; i < argc; i++) {
            size_t len_str = strlen(argv[i]); //length of argument
            if (len_str == 2) {
                if (argv[i][0] == '-' && argv[i][1] == '-') {
                    //find --
                    break;
                }
            }
            parse_argument(argv[i], len_str);
        }

        if (i == argc || i + 1 == argc) {
            //Cannot find -- in arguments
            //Cannot find cmd after --
            print_env_usage();
        }
        i++;
        execvp(argv[i], argv + i);
        print_exec_failed();
    } else {
        //is parent
        int status = 0;
        pid_t w = waitpid(child, &status, 0);
        if (w == -1) {
            exit(1);
        }
    }
    return 0;
}
