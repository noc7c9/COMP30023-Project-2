/*
 * COMP30023 Computer Systems Project 2
 * Ibrahim Athir Saleem (isaleem) (682989)
 *
 * Please see the corresponding header file for documentation on the module.
 *
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>

#include "server.h"

#define CONNECTION_BACKLOG 10

int server(int port, ConnectionHandler handler) {
    // create tcp socket
    int listener_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (listener_socket < 0) {
        perror("ERROR: opening socket");
        return 1;
    }

    // initialize server address struct
    struct sockaddr_in server_addr;
    bzero((char *) &server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    // allow reusing ports
    int yes = 1;
    if (-1 == setsockopt(listener_socket,
                SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))) {
        perror("ERROR: on setsockopt");
        return 1;
    }

    // bind
    if (bind(listener_socket,
                (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        close(listener_socket);
        perror("ERROR: on binding");
        return 1;
    }

    // listen
    if (-1 == listen(listener_socket, CONNECTION_BACKLOG)) {
        close(listener_socket);
        perror("ERROR: on listening");
        return 1;
    }

    // the infinite accept loop
    // all incoming connections are handed off to the given handler
    struct sockaddr_in client_addr;
    int client_socket;
    socklen_t client_addr_len;
    while (1) {
        client_addr_len = sizeof(client_addr);
        client_socket = accept(listener_socket,
                (struct sockaddr *) &client_addr, &client_addr_len);
        if (client_socket == -1) {
            // log the error, but then continue
            perror("ERROR: on accept");
            continue;
        }

        // hand the socket to the handler
        handler(client_socket);

        // done! close the socket then listen for more
        close(client_socket);
    }
}
