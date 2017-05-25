/*
 * COMP30023 Computer Systems Project 2
 * Ibrahim Athir Saleem (isaleem) (682989)
 *
 * A very basic thread safe queue module.
 *
 * Based on the example job queue from http://advancedlinuxprogramming.com
 *
 */

#pragma once

#include "linked_list.h"

/*
 * Struct for a queue.
 * Internals are private.
 */
typedef struct Queue Queue;

/*
 * Create a new empty queue.
 */
Queue *queue_init();

/*
 * Adds a new node onto the queue.
 * Returns the new node.
 */
Node *queue_enqueue(Queue *queue, void *data);

/*
 * Removes a node from the queue.
 * Returns the data stored in that node.
 * Note: If the queue is empty this will block until there is something on the
 *       queue.
 */
void *queue_dequeue(Queue *queue);

/*
 * Used to deallocate the given queue.
 * Warning: This will not free the contents of Node->data.
 */
void queue_destroy(Queue *queue);

/*
 * Runs the given function on every element of the queue.
 * Passes second_param as the second parameter to the function.
 */
void queue_iter(Queue *queue, void (*func)(void*, void*), void *second_param);
