/*
 * COMP30023 Computer Systems Project 2
 * Ibrahim Athir Saleem (isaleem) (682989)
 *
 * Please see the corresponding header file for documentation on the module.
 *
 */

#include <stdlib.h>
#include <assert.h>
#include <pthread.h>
#include <semaphore.h>

#include "queue.h"


/***** Private structs
 */

struct Queue {
    LinkedList *ll;
    pthread_mutex_t mutex;
    sem_t count;
};


/***** Public functions
 */

Queue *queue_init() {
    Queue *queue = (Queue *) malloc(sizeof(Queue));
    assert(queue);

    queue->ll = linked_list_init();
    pthread_mutex_init(&queue->mutex, NULL);
    sem_init(&queue->count, 0, 0);

    return queue;
}

Node *queue_enqueue(Queue *queue, void *data) {
    Node *new_node = NULL;

    pthread_mutex_lock(&queue->mutex);

    new_node = linked_list_push_start(queue->ll, data);

    sem_post(&queue->count);

    pthread_mutex_unlock(&queue->mutex);

    return new_node;
}

void *queue_dequeue(Queue *queue) {
    void *data = NULL;

    // block until there is something on the queue
    sem_wait(&queue->count);

    pthread_mutex_lock(&queue->mutex);

    data = linked_list_pop_end(queue->ll);

    pthread_mutex_unlock(&queue->mutex);

    return data;
}

void queue_destroy(Queue *queue) {
    pthread_mutex_lock(&queue->mutex);
    linked_list_destroy(queue->ll);
    pthread_mutex_unlock(&queue->mutex);

    pthread_mutex_destroy(&queue->mutex);
    sem_destroy(&queue->count);

    free(queue);
}

void queue_iter(Queue *queue, void (*func)(void*)) {
    if (func == NULL) {
        return;
    }

    pthread_mutex_lock(&queue->mutex);

    for (Node *n = queue->ll->head; n != NULL; n = n->next) {
        func(n->data);
    }

    pthread_mutex_unlock(&queue->mutex);
}
