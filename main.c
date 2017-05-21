/*
 * COMP30023 Computer Systems Project 2
 * Ibrahim Athir Saleem (isaleem) (682989)
 *
 * Hashcash proof-of-work solver server.
 *
 * Usage: ./server PORT_NUMBER
 *   PORT_NUMBER: port number to connect to,
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <assert.h>

#include "server.h"

#define BUFLEN 1024


/*
 * Handles communication with each individual client (ie each connection).
 */
void *client_handler(void *psockfd) {
    int sockfd = *((int *) psockfd);
    free(psockfd);

    char buffer[BUFLEN + 1];
    bzero(buffer, BUFLEN);
    int read_n;

    while (0 != (read_n = read(sockfd, buffer, BUFLEN))) {
        if (read_n < 0) {
            perror("ERROR: reading from socket");
            return NULL;
        }

        buffer[read_n-1] = '\0'; // overwrites the newline

        printf("Read message from client %d of len %d: [%s]\n", sockfd, read_n, buffer);

        if (0 > write(sockfd, buffer, read_n)) {
            perror("ERROR: writing to socket");
            return NULL;
        }
    }

    printf("Client %d disconnected\n", sockfd);
    close(sockfd);

    return NULL;
}


/*
 * A server handler function that spawns a detached thread to actually handle
 * each connection.
 */
void handler_thread_spawner(int sockfd) {
    // passing the sockfd in a proper manner
    int *psockfd = malloc(sizeof(int));
    assert(NULL != psockfd);
    *psockfd = sockfd;

    // threads should be created detached, as they don't return anything
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    // create the thread
    pthread_t tid;
    pthread_create(&tid, NULL, client_handler, (void *) psockfd);

    // clean up
    pthread_attr_destroy(&attr);
}


int main(int argc, char *argv[]) {
    int port;

    if (argc < 2) {
        fprintf(stderr, "ERROR: no port provided\n");
        exit(1);
    }

    port = atoi(argv[1]);

    server(port, handler_thread_spawner);

    return 0;
}
