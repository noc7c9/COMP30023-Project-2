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
#include "sstp.h"


/*
 * Handles communication with each individual client (ie each connection).
 */
void *client_handler(void *psockfd) {
    int sockfd = *((int *) psockfd);
    free(psockfd);

    SSTPStream *sstp = sstp_init(sockfd);

    SSTPMsg msg;

    int res;
    while (0 != (res = sstp_read(sstp, &msg))) {
        if (res < 0) {
            perror("ERROR: reading from socket");
            return NULL;
        }

        switch (msg.type) {
            case PING:
                sstp_write(sstp, PONG, NULL);
                break;
            case PONG:
                sstp_write(sstp, ERRO,
                        "PONG msgs are reserved for the server.");
                break;
            case OKAY:
                sstp_write(sstp, ERRO,
                        "OKAY msgs are reserved for the server.");
                break;
            case ERRO:
                sstp_write(sstp, ERRO,
                        "ERRO msgs are reserved for the server.");
                break;
            default:
                sstp_write(sstp, ERRO,
                        "Malformed message.");
                break;
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
