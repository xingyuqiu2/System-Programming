/**
 * deadlock_demolition
 * CS 241 - Spring 2021
 */
#include "graph.h"
#include "libdrm.h"
#include "set.h"
#include <pthread.h>

struct drm_t { pthread_mutex_t m; };
static pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
static graph* g;
static set* s;

int detect_cycle(void* cur);

drm_t *drm_init() {
    /* Your code here */
    drm_t* drm = malloc(sizeof(drm_t));
    if (!drm) return NULL;
    pthread_mutex_init(&(drm->m), NULL);

    pthread_mutex_lock(&m);
    if (!g) g = shallow_graph_create();
    graph_add_vertex(g, drm);
    pthread_mutex_unlock(&m);
    return drm;
}

int drm_post(drm_t *drm, pthread_t *thread_id) {
    /* Your code here */
    int res = 0;
    pthread_mutex_lock(&m);
    //Check to see if the vertex is in the graph
    //and if an edge from the drm to the thread exists, remove the edge and unlock the drm_t
    if (graph_contains_vertex(g, thread_id) && graph_contains_vertex(g, drm) && graph_adjacent(g, drm, thread_id)) {
        res = 1;
        graph_remove_edge(g, drm, thread_id);
        pthread_mutex_unlock(&(drm->m));
    }
    pthread_mutex_unlock(&m);
    return res;
}

int drm_wait(drm_t *drm, pthread_t *thread_id) {
    /* Your code here */
    int res = 0;
    pthread_mutex_lock(&m);
    //Add the thread to the Resource Allocation Graph if not already present.
    if (!graph_contains_vertex(g, thread_id)) {
        graph_add_vertex(g, thread_id);
    }
    //thread trying to lock a mutex it already owns
    if (graph_contains_vertex(g, drm) && graph_adjacent(g, drm, thread_id)) {
        pthread_mutex_unlock(&m);
        return res;
    }
    //detect if the newly added edge creates a cycle in the Resource Allocation Graph.
    graph_add_edge(g, thread_id, drm);
    int has_cycle = detect_cycle(thread_id);

    if (has_cycle) {
        //If the attempt to lock the drm would cause a deadlock, then reject the locking attempt by returning without locking the drm.
        graph_remove_edge(g, thread_id, drm);
        pthread_mutex_unlock(&m);
    } else {
        //If not, then lock the drm.
        res = 1;
        pthread_mutex_unlock(&m);
        pthread_mutex_lock(&(drm->m));

        pthread_mutex_lock(&m);
        graph_remove_edge(g, thread_id, drm);
        graph_add_edge(g, drm, thread_id);
        pthread_mutex_unlock(&m);
    }
    return res;
}

void drm_destroy(drm_t *drm) {
    /* Your code here */
    pthread_mutex_lock(&m);
    graph_remove_vertex(g, drm);
    pthread_mutex_unlock(&m);
    pthread_mutex_destroy(&(drm->m));
    free(drm);
    return;
}

int detect_cycle(void* cur) {
    if (!s) s = shallow_set_create();
    if (set_contains(s, cur)) {
        set_destroy(s);
        s = NULL;
        return 1;
    }
    set_add(s, cur);
    vector* neighbors = graph_neighbors(g, cur);
    for (size_t i = 0; i < vector_size(neighbors); i++) {
        if (detect_cycle(vector_get(neighbors, i))) {
            return 1;
        }
    }
    vector_destroy(neighbors);
    set_destroy(s);
    s = NULL;
    return 0;
}
