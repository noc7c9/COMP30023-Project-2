/*
 * COMP30023 Computer Systems Project 2
 * Ibrahim Athir Saleem (isaleem) (682989)
 *
 * The server module. Abstracts all the socker boilerplate for creating a local
 * server that listens on the given port.
 * Actual communication is handed off to a connection handler that the server
 * takes as an argument.
 *
 */

#pragma once

/*
 * Typedef for the connection handlers which are function pointers,
 * that handle communication with clients.
 */
typedef void (*ConnectionHandler)(int sockfd);

/*
 * The main server, takes the port to connect to and a connection handler to
 * use.
 * Returns non-zero if an error occurs.
 */
int server(int port, ConnectionHandler handler);
