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

#include "server.h"

void basic_handler(int sockfd) {
    char buffer[256];
    bzero(buffer, 256);
    int n = read(sockfd, buffer, 255);

    if (n < 0) {
        perror("ERROR: reading from socket");
        return;
    }

    printf("Here is the message: [%s]\n", buffer);

    n = write(sockfd, "I got your message\n", 19);

    if (n < 0) {
        perror("ERROR: writing to socket");
        return;
    }

    close(sockfd);
}

int main(int argc, char *argv[]) {
    int port;

    if (argc < 2) {
        fprintf(stderr, "ERROR: no port provided\n");
        exit(1);
    }

    port = atoi(argv[1]);

    server(port, basic_handler);

    return 0;
}
