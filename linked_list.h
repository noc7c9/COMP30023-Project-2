/*
 * COMP30023 Computer Systems Project 1
 * Ibrahim Athir Saleem (isaleem) (682989)
 *
 * A generic doubly linked list implementation. Uses a Node struct as the
 * elements of the list, each Node having a void* field called data for
 * external data.
 *
 */

#pragma once

/*
 * The node struct.
 */
typedef struct Node Node;
struct Node {
    void *data;
    Node *next;
    Node *prev;
};

/*
 * The top level container struct.
 */
typedef struct {
    Node *head;
    Node *tail;
    int len;
} LinkedList;

/*
 * Create a new empty linked list instance.
 */
LinkedList *linked_list_init();

/*
 * Returns true if there are no nodes in the list.
 */
int linked_list_is_empty(LinkedList *ll);

/*
 * Create a new node at the start of list.
 * Returns the new node.
 */
Node *linked_list_push_start(LinkedList *ll, void *data);

/*
 * Create a new node at the end of list.
 * Returns the new node.
 */
Node *linked_list_push_end(LinkedList *ll, void *data);

/*
 * Create a new node right after the given target node.
 * Returns the new node.
 */
Node *linked_list_insert_after(LinkedList *ll, Node *target, void *data);

/*
 * Remove and return the node at the start of list.
 */
void *linked_list_pop_start(LinkedList *ll);

/*
 * Remove and return the node at the end of list.
 */
void *linked_list_pop_end(LinkedList *ll);

/*
 * Removes and returns the given node from the list.
 * ie. pop from anywhere in the list.
 */
void *linked_list_pop(LinkedList *ll, Node *node);

/*
 * Used to deallocate the given linked list.
 * Warning: This will not free the contents of Node->data.
 */
void linked_list_destroy(LinkedList *ll);

/*
 * Print out the linked list and its contents.
 * The print_node function pointer can be NULL, in which case the void* pointer
 * will be printed out directly.
 * For use in debugging.
 */
void linked_list_print(LinkedList *ll, void (*print_node)(void*));
