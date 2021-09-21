/**
 * mini_memcheck
 * CS 241 - Spring 2021
 */
#include "mini_memcheck.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int is_valid_ptr(void* ptr, meta_data** prev_entry);

meta_data* head;
size_t total_memory_requested;
size_t total_memory_freed;
size_t invalid_addresses;

void *mini_malloc(size_t request_size, const char *filename,
                  void *instruction) {
    // your code here
    if (request_size == 0) return NULL;
    meta_data* entry = malloc(sizeof(meta_data) + request_size);
    if (!entry) return NULL;
    entry->request_size = request_size;
    entry->filename = filename;
    entry->instruction = instruction;
    entry->next = NULL;
    if (!head) {
        head = entry;
    } else {
        meta_data* next = head;
        head = entry;
        entry->next = next;
    }
    total_memory_requested += request_size;
    return (void*) (entry + 1);
}

void *mini_calloc(size_t num_elements, size_t element_size,
                  const char *filename, void *instruction) {
    // your code here
    size_t request_size = num_elements * element_size;
    void* ptr = mini_malloc(request_size, filename, instruction);
    if (!ptr) return NULL;
    //reset memory to 0
    memset(ptr, 0, request_size);
    return ptr;
}

void *mini_realloc(void *payload, size_t request_size, const char *filename,
                   void *instruction) {
    // your code here
    if (!payload) return mini_malloc(request_size, filename, instruction);
    if (request_size == 0) {
        mini_free(payload);
        return NULL;
    }
    meta_data* prev_entry = NULL;
    if (!is_valid_ptr(payload, &prev_entry)) {
        prev_entry = NULL;
        invalid_addresses++;
        return NULL;
    }
    meta_data* cur_entry = (meta_data*) payload - 1;
    size_t oldsize = cur_entry->request_size;
    if (oldsize == request_size) {
        cur_entry->filename = filename;
        cur_entry->instruction = instruction;
        return payload;
    }
    meta_data* new_entry = realloc(cur_entry, sizeof(meta_data) + request_size);
    if (!new_entry) return NULL;
    if (oldsize < request_size) {
        total_memory_requested += request_size - oldsize;
    } else {
        total_memory_freed += oldsize - request_size;
    }
    new_entry->request_size = request_size;
    new_entry->filename = filename;
    new_entry->instruction = instruction;
    //set the next of prev_entry to new_entry
    if (prev_entry) {
        prev_entry->next = new_entry;
        prev_entry = NULL;
    } else {
        head = new_entry;
    }
    return (void*) (new_entry + 1);
}

void mini_free(void *payload) {
    // your code here
    if (!payload) return;
    meta_data* prev_entry = NULL;
    if (!is_valid_ptr(payload, &prev_entry)) {
        prev_entry = NULL;
        invalid_addresses++;
        return;
    }
    meta_data* cur_entry = (meta_data*) payload - 1;
    //set the next of prev_entry to next of cur_entry
    if (prev_entry) {
        prev_entry->next = cur_entry->next;
        prev_entry = NULL;
    } else {
        head = cur_entry->next;
    }
    total_memory_freed += cur_entry->request_size;
    free(cur_entry);
}

int is_valid_ptr(void* ptr, meta_data** prev_entry) {
    //to find if there is one entry for this address
    if (!ptr) return 0;
    if (!head) return 0;
    meta_data* cur = head;
    while (cur && (void*) (cur + 1) != ptr) {
        *prev_entry = cur;
        cur = cur->next;
    }
    if (!cur) return 0;
    return 1;
}
