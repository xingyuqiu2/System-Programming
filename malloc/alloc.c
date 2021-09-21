/**
 * malloc
 * CS 241 - Spring 2021
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define SPLIT_CONSTANT 1.5
#define SPLIT_THRESHOLD 3e8

typedef struct _meta_data {
    // Number of bytes of heap memory the user requested from malloc
    size_t size;
    // flag whether the data is freed
    char free;
    // Pointer to the previous instance of meta_data in the list
    struct _meta_data *prev;
    // Pointer to the next instance of meta_data in the list
    struct _meta_data *next;
} meta_data;

static meta_data *head = NULL;  //store all the freed metadata
static void* start = NULL;  //start of the heap address, sbrk(0) of the first malloc

void split_metadata(meta_data* ptr, size_t oldsize, size_t newsize);
void merge_prev_cur_next(meta_data* cur);
void merge_cur_next(meta_data* ptr);
void merge_prev_cur(meta_data* cur);

/**
 * Allocate space for array in memory
 *
 * Allocates a block of memory for an array of num elements, each of them size
 * bytes long, and initializes all its bits to zero. The effective result is
 * the allocation of an zero-initialized memory block of (num * size) bytes.
 *
 * @param num
 *    Number of elements to be allocated.
 * @param size
 *    Size of elements.
 *
 * @return
 *    A pointer to the memory block allocated by the function.
 *
 *    The type of this pointer is always void*, which can be cast to the
 *    desired type of data pointer in order to be dereferenceable.
 *
 *    If the function failed to allocate the requested block of memory, a
 *    NULL pointer is returned.
 *
 * @see http://www.cplusplus.com/reference/clibrary/cstdlib/calloc/
 */
void *calloc(size_t num, size_t size) {
    // implement calloc!
    size_t request_size = num * size;
    void* ptr = malloc(request_size);
    if (!ptr) return NULL;
    //reset memory to 0
    memset(ptr, 0, request_size);
    return ptr;
}

/**
 * Allocate memory block
 *
 * Allocates a block of size bytes of memory, returning a pointer to the
 * beginning of the block.  The content of the newly allocated block of
 * memory is not initialized, remaining with indeterminate values.
 *
 * @param size
 *    Size of the memory block, in bytes.
 *
 * @return
 *    On success, a pointer to the memory block allocated by the function.
 *
 *    The type of this pointer is always void*, which can be cast to the
 *    desired type of data pointer in order to be dereferenceable.
 *
 *    If the function failed to allocate the requested block of memory,
 *    a null pointer is returned.
 *
 * @see http://www.cplusplus.com/reference/clibrary/cstdlib/malloc/
 */
void *malloc(size_t size) {
    // implement malloc!
    if (start == NULL) start = sbrk(0);
    if (size == 0) return NULL;
    meta_data* cur = head;
    meta_data* chosen = NULL;

    //see if we have free block of enough size
    while (cur) {
        if (cur->size >= size) {
            if (!chosen || cur->size < chosen->size) {
                chosen = cur;
            }
        }
        cur = cur->next;
    }
    if (chosen) {
        //we find a free block of enough size
        size_t oldsize = chosen->size;
        chosen->free = 0;
        //see if need to split the space
        if (oldsize > SPLIT_CONSTANT * size && oldsize - size > sizeof(meta_data) + sizeof(size_t)) {
            split_metadata(chosen, oldsize, size);
            return (void*) (chosen + 1);
        }
    } else {
        //request new space in heap
        chosen = sbrk(sizeof(meta_data) + size + sizeof(size_t));
        if ((void*)chosen == (void*)-1) return NULL;
        chosen->size = size;
        chosen->free = 0;
        chosen->prev = NULL;
        chosen->next = head;
        //mark size at the end of the data
        size_t* size_mark = (void*)chosen + sizeof(meta_data) + size;
        *size_mark = size;
    }
    //try to remove the chosen block from the linked list by connecting chosen->prev to chosen->next
    if (chosen->prev && chosen->next) {
        chosen->prev->next = chosen->next;
        chosen->next->prev = chosen->prev;
    } else if (chosen->prev) {
        chosen->prev->next = NULL;
    } else if (chosen->next) {
        chosen->next->prev = NULL;
        head = chosen->next;
    } else {
        //no freed blocks available, set head to NULL
        head = NULL;
    }
    return (void*) (chosen + 1);
}

/**
 * Deallocate space in memory
 *
 * A block of memory previously allocated using a call to malloc(),
 * calloc() or realloc() is deallocated, making it available again for
 * further allocations.
 *
 * Notice that this function leaves the value of ptr unchanged, hence
 * it still points to the same (now invalid) location, and not to the
 * null pointer.
 *
 * @param ptr
 *    Pointer to a memory block previously allocated with malloc(),
 *    calloc() or realloc() to be deallocated.  If a null pointer is
 *    passed as argument, no action occurs.
 */
void free(void *ptr) {
    // implement free!
    if (!ptr) return;
    meta_data* cur = (meta_data*) ptr - 1;
    cur->free = 1;

    if (cur->size <= 1024) {
        //no need to merge, then add cur to the head of the linked list that stores the freed blocks
        if (head == NULL) {
			head = cur;
			cur->next = NULL;
			cur->prev = NULL;
		} else {
			cur->next = head;
			cur->prev = NULL;
			head->prev = cur;
			head = cur;
		}
        return;
    }
    //find the possible prev and next metadata (in continuous addresses) and set them to cur->prev and cur->next
    size_t* prev_size_mark = (void*)cur - sizeof(size_t);
    cur->prev = (void*)prev_size_mark - *prev_size_mark - sizeof(meta_data);
    cur->next = ptr + cur->size + sizeof(size_t);
    //check if the prev and next metadata are within the heap
    int has_prev = 0;
    int has_next = 0;
    void* end = sbrk(0) - sizeof(meta_data) - sizeof(size_t);
    if ((void*)cur->prev >= start && (void*)cur->prev <= end) has_prev = 1;
    if ((void*)cur->next >= start && (void*)cur->next <= end) has_next = 1;
    
    //try to merge freed meta_data
    if (has_next && cur->next->free == 1 && has_prev && cur->prev->free == 1) {
        //merge with prev and next meta_data
        merge_prev_cur_next(cur);
    } else if (has_next && cur->next->free == 1) {
        //merge with next meta_data
        merge_cur_next(cur);
    } else if (has_prev && cur->prev->free == 1) {
        //merge with prev meta_data
        merge_prev_cur(cur);
    } else {
        //no need to merge, then add cur to the head of the linked list that stores the freed blocks
        if (head == NULL) {
			head = cur;
			cur->next = NULL;
			cur->prev = NULL;
		} else {
			cur->next = head;
			cur->prev = NULL;
			head->prev = cur;
			head = cur;
		}
    }
}

/**
 * Reallocate memory block
 *
 * The size of the memory block pointed to by the ptr parameter is changed
 * to the size bytes, expanding or reducing the amount of memory available
 * in the block.
 *
 * The function may move the memory block to a new location, in which case
 * the new location is returned. The content of the memory block is preserved
 * up to the lesser of the new and old sizes, even if the block is moved. If
 * the new size is larger, the value of the newly allocated portion is
 * indeterminate.
 *
 * In case that ptr is NULL, the function behaves exactly as malloc, assigning
 * a new block of size bytes and returning a pointer to the beginning of it.
 *
 * In case that the size is 0, the memory previously allocated in ptr is
 * deallocated as if a call to free was made, and a NULL pointer is returned.
 *
 * @param ptr
 *    Pointer to a memory block previously allocated with malloc(), calloc()
 *    or realloc() to be reallocated.
 *
 *    If this is NULL, a new block is allocated and a pointer to it is
 *    returned by the function.
 *
 * @param size
 *    New size for the memory block, in bytes.
 *
 *    If it is 0 and ptr points to an existing block of memory, the memory
 *    block pointed by ptr is deallocated and a NULL pointer is returned.
 *
 * @return
 *    A pointer to the reallocated memory block, which may be either the
 *    same as the ptr argument or a new location.
 *
 *    The type of this pointer is void*, which can be cast to the desired
 *    type of data pointer in order to be dereferenceable.
 *
 *    If the function failed to allocate the requested block of memory,
 *    a NULL pointer is returned, and the memory block pointed to by
 *    argument ptr is left unchanged.
 *
 * @see http://www.cplusplus.com/reference/clibrary/cstdlib/realloc/
 */
void *realloc(void *ptr, size_t size) {
    // implement realloc!
    if (!ptr) return malloc(size);
    if (size == 0) {
        free(ptr);
        return NULL;
    }
    
    meta_data* cur = (meta_data*)ptr - 1;
    size_t oldsize = cur->size;
    //new size is smaller than old size
    if (oldsize >= size) {
        if (oldsize <= SPLIT_THRESHOLD) return ptr;
        //see if need to split the space
        if (oldsize > SPLIT_CONSTANT * size && oldsize - size > sizeof(meta_data) + sizeof(size_t)) {
            meta_data* newentry = (void*)(cur + 1) + size + sizeof(size_t);
            newentry->size = oldsize - size - sizeof(meta_data) - sizeof(size_t);
            newentry->free = 1;

            cur->size = size;
            cur->free = 0;
            //update the first metadata and the size mark to store the new size information
            size_t* cur_size_mark = (void*)cur + sizeof(meta_data) + size;
            *cur_size_mark = size;
            size_t* newentry_size_mark = (void*)newentry + sizeof(meta_data) + newentry->size;
            *newentry_size_mark = newentry->size;

            if(head == NULL){
                head = newentry;
                newentry->next = NULL;
                newentry->prev = NULL;
            }else{
                newentry->next = head;
                newentry->prev = NULL;
                head->prev = newentry;
                head = newentry;
		    }
        }
        return ptr;
    }
    //new size is greater than old size
    void* new_ptr = malloc(size);
    if (!new_ptr) return NULL;
    memcpy(new_ptr, ptr, oldsize);
    free(ptr);
    return new_ptr;
}

//split the current data into two blocks of data so that current block is allocated, next block is free 
void split_metadata(meta_data* cur, size_t oldsize, size_t newsize) {
    meta_data* newentry = (void*)(cur + 1) + newsize + sizeof(size_t);
    newentry->size = oldsize - newsize - sizeof(meta_data) - sizeof(size_t);
    newentry->free = 1;
    newentry->prev = cur->prev;
    newentry->next = cur->next;
    //update the first metadata and the size mark to store the new size information
    size_t* cur_size_mark = (void*)cur + sizeof(meta_data) + newsize;
    *cur_size_mark = newsize;
    size_t* newentry_size_mark = (void*)newentry + sizeof(meta_data) + newentry->size;
    *newentry_size_mark = newentry->size;
    //try to remove the current block from the linked list of free blocks
    //by setting the cur->prev->next and cur->next->prev to point to the new free block
    if (cur->prev && cur->next) {
        cur->prev->next = newentry;
        cur->next->prev = newentry;
    } else if (cur->prev) {
        cur->prev->next = newentry;
    } else if (cur->next) {
        cur->next->prev = newentry;
        head = newentry;
    } else {
        head = newentry;
    }

    cur->size = newsize;
    cur->free = 0;
}

//merge prev, cur, next blocks as one free block
void merge_prev_cur_next(meta_data* cur) {
    //update the first metadata and the size mark to store the new size information
    cur->prev->size += cur->size + cur->next->size + 2*sizeof(meta_data) + 2*sizeof(size_t);
    size_t* size_mark = (void*)cur->next + sizeof(meta_data) + cur->next->size;
    *size_mark = cur->prev->size;
    //try to remove the third block from the linked list by connecting its prev to next since it is united to the first free block
    //keep the relation of first free block with its neighbors since it is already in the linked list
    meta_data* third_block = cur->next;
    if (third_block->prev && third_block->next){
        third_block->prev->next = third_block->next;
        third_block->next->prev = third_block->prev;
    } else if (third_block->prev){
        third_block->prev->next = NULL;
    } else if (third_block->next){
        third_block->next->prev = NULL;
        head = third_block->next;
    } else {
        head = NULL;
    }
    cur->free = 0;
    third_block->free = 0;
}

//merge cur, next blocks as one free block
void merge_cur_next(meta_data* cur) {
    //update the cur metadata and the size mark to store the new size information
    cur->size += cur->next->size + sizeof(meta_data) + sizeof(size_t);
    size_t* size_mark = (void*)cur->next + sizeof(meta_data) + cur->next->size;
    *size_mark = cur->size;
    //try to set the cur block as one big free block and update the relation with its prev and next free blocks in the linked list
    //cur prev and next should be set to second_block's prev and next(since second_block is already in the linked list)
    meta_data* second_block = cur->next;
    cur->prev = second_block->prev;
	cur->next = second_block->next;
    //try to update the new cur->prev->next and new cur->next->prev to point to cur
    if(second_block->prev && second_block->next){
		second_block->prev->next = cur;
		second_block->next->prev = cur;
	}else if(second_block->prev){
		second_block->prev->next = cur;
	}else if(second_block->next){
		second_block->next->prev = cur;
		head = cur;
	}else{
		head = cur;
    }
    second_block->free = 0;
}

//merge prev, cur blocks as one free block
void merge_prev_cur(meta_data* cur) {
    //update the prev metadata and the size mark to store the new size information
    cur->prev->size += cur->size + sizeof(meta_data) + sizeof(size_t);
    size_t* size_mark = (void*)cur + sizeof(meta_data) + cur->size;
    *size_mark = cur->prev->size;
    cur->free = 0;
    //keep the relation of first free block with its neighbors since it is already in the linked list
}

