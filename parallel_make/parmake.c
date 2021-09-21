/**
 * parallel_make
 * CS 241 - Spring 2021
 */

#include "format.h"
#include "graph.h"
#include "parmake.h"
#include "parser.h"
#include "set.h"
#include "queue.h"
#include "vector.h"
#include "dictionary.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>

static graph* g;    //dependency graph
static set* s;
static vector* rules_vector;

static pthread_mutex_t m_rule = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t m_graph = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cv = PTHREAD_COND_INITIALIZER;

//check whether the graph beginning at cur has a cycle
int has_cycle(void* cur) {
    if (!s) s = shallow_set_create();
    if (set_contains(s, cur)) {
        set_destroy(s);
        s = NULL;
        return 1;
    }
    int res = 0;
    set_add(s, cur);
    vector* neighbors = graph_neighbors(g, cur);
    for (size_t i = 0; i < vector_size(neighbors); i++) {
        if (has_cycle(vector_get(neighbors, i))) {
            res = 1;
            break;
        }
    }
    vector_destroy(neighbors);
    if (s) {
        set_destroy(s);
        s = NULL;
    }
    return res;
}

//get the ordered rules from the descendants of a goal rule and store them in rules_queue (inefficient order, but correct to match the order for test10)
void get_ordered_rules(void* cur) {
    vector* neighbors = graph_neighbors(g, cur);
    for (size_t i = 0; i < vector_size(neighbors); i++) {
        void* target = vector_get(neighbors, i);
        get_ordered_rules(target);
    }
    if (!set_contains(s, cur)) {
        set_add(s, cur);
        vector_push_back(rules_vector, cur);
    }
    vector_destroy(neighbors);
}

// get the rules_vector in reverse order
// void get_ordered_rules_vector(vector* goal_rules) {
//     rules_vector = string_vector_create();
//     vector* temp_vector = string_vector_create();   //temp rules_vector with repeated keys
//     set* repeat_idx_set = int_set_create();     //position of repeated keys that need not to be in the rules_vector
//     dictionary* dic_key_idx = string_to_int_dictionary_create();    //dic {key -> position}
//     size_t pre_level_size = 0;
//     size_t pre_level_step = 0;
//     size_t cur_level_step = 0;
//     size_t cur_idx = 0;
//     for (int i = vector_size(goal_rules) - 1; i >= 0; i--) {
//         void* cur = vector_get(goal_rules, i);
//         if (dictionary_contains(dic_key_idx, cur)) {
//             int* pos = dictionary_get(dic_key_idx, cur);
//             int pre_idx = *pos;
//             set_add(repeat_idx_set, &pre_idx);
//         }
//         int idx = vector_size(temp_vector);
//         dictionary_set(dic_key_idx, cur, &idx);
//         vector_push_back(temp_vector, cur);
//         pre_level_size++;
//     }
//     while (cur_idx != vector_size(temp_vector)) {
//         while (pre_level_step < pre_level_size) {
//             void* parent_rule = vector_get(temp_vector, cur_idx);
//             vector* neighbors = graph_neighbors(g, parent_rule);
//             for (int i = vector_size(neighbors) - 1; i >= 0; i--) {
//                 void* cur = vector_get(neighbors, i);
//                 if (dictionary_contains(dic_key_idx, cur)) {
//                     int* pos = dictionary_get(dic_key_idx, cur);
//                     int pre_idx = *pos;
//                     set_add(repeat_idx_set, &pre_idx);
//                 }
//                 int idx = vector_size(temp_vector);
//                 dictionary_set(dic_key_idx, cur, &idx);
//                 vector_push_back(temp_vector, cur);
//                 cur_level_step++;
//             }
//             vector_destroy(neighbors);
//             pre_level_step++;
//             cur_idx++;
//         }
//         pre_level_size = cur_level_step;
//         cur_level_step = 0;
//         pre_level_step = 0;
//     }
//     for (int i = 0; i < (int)vector_size(temp_vector); i++) {
//         //printf("%s\n", (char*)vector_get(temp_vector, i));
//         if (!set_contains(repeat_idx_set, &i)) {
//             //if key is in the greatest height position, then push it to the rules_vector
//             vector_push_back(rules_vector, vector_get(temp_vector, i));
//         }
//     }
//     vector_destroy(temp_vector);
//     set_destroy(repeat_idx_set);
//     dictionary_destroy(dic_key_idx);
// }

//function for threads
void* run(void* ptr) {
    while (1) {
        pthread_mutex_lock(&m_rule);
        if (vector_size(rules_vector) == 0) {
            pthread_mutex_unlock(&m_rule);
            break;
        }
        void* target = NULL;
        vector* neighbors = NULL;
        rule_t* rule = NULL;
        int state = 0;
        size_t idx = 0;
        size_t ori_size = vector_size(rules_vector);
        pthread_mutex_lock(&m_graph);
        for (; idx < vector_size(rules_vector); idx++) {
            char* temp = vector_get(rules_vector, idx);
            target = malloc(strlen(temp) + 1);
            target = strncpy(target, temp, strlen(temp) + 1);
            state = 0;
            rule = (rule_t*)graph_get_vertex_value(g, target);
            //if any of the dependencies failed, then mark the rule as failed and don't run
            neighbors = graph_neighbors(g, target);
            size_t neighbors_size = vector_size(neighbors);
            if (neighbors_size != 0) {
                int check_next = 0; //true if dependencies have not finished
                for (size_t i = 0; i < vector_size(neighbors); i++) {
                    void* cur_target = vector_get(neighbors, i);
                    rule_t* pre_rule = (rule_t*)graph_get_vertex_value(g, cur_target);
                    if (pre_rule->state == 0) {
                        check_next = 1;
                        break;
                    }
                    if (pre_rule->state == -1) {
                        state = -1;
                        break;
                    }
                }
                if (check_next) {
                    free(target);
                    target = NULL;
                    vector_destroy(neighbors);
                    continue;
                }
            }
            vector_erase(rules_vector, idx);
            break;
        }
        pthread_mutex_unlock(&m_graph);
        if (idx == ori_size) {
            //no available rules
            pthread_cond_wait(&cv, &m_rule);
            pthread_mutex_unlock(&m_rule);
            continue;
        }
        //pthread_mutex_unlock(&m_rule);

        if (state != 0) {
            pthread_mutex_unlock(&m_rule);
            if (target) {
                free(target);
                target = NULL;
            }
            vector_destroy(neighbors);
            pthread_mutex_lock(&m_graph);
            rule->state = state;
            pthread_cond_broadcast(&cv);
            pthread_mutex_unlock(&m_graph);
            continue;
        }
        //is the rule the name of a file on disk?
        if (access((char*)target, F_OK) == 0) { //YES
            for (size_t i = 0; i < vector_size(neighbors); i++) {
                //Does the rule depend on another rule that is a file on disk?
                void* cur_target = vector_get(neighbors, i);
                if (access((char*)cur_target, F_OK) == 0) { //YES
                    //Does any of the rule's dependecies have a newer modification time than the rule's modification time
                    struct stat stat0, stat1;
                    // failed to read file's stat
                    if (stat((char*)target, &stat0) == -1 || stat((char*)cur_target, &stat1) == -1) {
                        state = -1;
                        break;
                    }   
                    // if cur_target is not newer than target
                    if (difftime(stat1.st_mtime, stat0.st_mtime) < 0) {
                        //mark the rule as satisfied
                        state = 1;
                        break;
                    }
                }
            }
        }
        pthread_mutex_unlock(&m_rule);
        if (target) {
            free(target);
            target = NULL;
        }
        vector_destroy(neighbors);
        if (state != 0) {
            pthread_mutex_lock(&m_graph);
            rule->state = state;
            pthread_cond_broadcast(&cv);
            pthread_mutex_unlock(&m_graph);
            continue;
        }
        //run the rule's commands one by one
        vector* commands = rule->commands;
        for (size_t i = 0; i < vector_size(commands); i++) {
            //if command failed
            if (system((char*)vector_get(commands, i)) != 0) {
                //mark the rule as failed
                state = -1;
                break;
            }
        }
        //mark the rule as satisfied
        if (state == 0) state = 1;
        pthread_mutex_lock(&m_graph);
        rule->state = state;
        pthread_cond_broadcast(&cv);
        pthread_mutex_unlock(&m_graph);
    }
    pthread_exit(NULL);
}

int parmake(char *makefile, size_t num_threads, char **targets) {
    // good luck!
    pthread_t tid[num_threads];

    g = parser_parse_makefile(makefile, targets);
    vector* goal_rules = graph_neighbors(g, "");
    rules_vector = string_vector_create();

    //check cycles for every goal rule
    size_t i = 0;
    while (i < vector_size(goal_rules)) {
        void* cur = vector_get(goal_rules, i);
        if (has_cycle(cur)) {
            print_cycle_failure((char*)cur);
            //ignore all goal rules whose descendants belong to cycles
            vector_erase(goal_rules, i);
        } else {
            i++;
        }
    }

    //if all goal rules have descendants belong to cycles, return
    if (vector_empty(goal_rules)) {
        vector_destroy(rules_vector);
        vector_destroy(goal_rules);
        graph_destroy(g);
        return 0;
    }

    //get the ordered rules_queue
    // get_ordered_rules_vector(goal_rules);
    // for (int idx = vector_size(rules_vector) - 1; idx >= 0; idx--) {
    //     void* cur = vector_get(rules_vector, idx);
    //     queue_push(rules_queue, cur);
    // }
    // count = (int)vector_size(rules_vector);
    s = shallow_set_create();
    for (i = 0; i < vector_size(goal_rules); i++) {
        void* target = vector_get(goal_rules, i);
        get_ordered_rules(target);
    }
    set_destroy(s);

    //try running the commands
    for (i = 0; i < num_threads; i++) {
        pthread_create(tid + i, NULL, run, NULL);
    }
    for (i = 0; i < num_threads; i++) {
        pthread_join(tid[i], NULL);
    }

    //free memory
    vector_destroy(rules_vector);
    vector_destroy(goal_rules);
    graph_destroy(g);
    return 0;
}
