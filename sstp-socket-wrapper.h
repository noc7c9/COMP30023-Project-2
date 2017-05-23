/*
 * COMP30023 Computer Systems Project 2
 * Ibrahim Athir Saleem (isaleem) (682989)
 *
 * The module that provides functions for reading from and writing to a network
 * socket using the Simple Stratum Text Protocol (SSTP).
 *
 */

#pragma once

#include "sstp.h"

/*
 * The struct to store the state of the socket.
 * Internals are private.
 */
typedef struct SSTPStream SSTPStream;

/*
 * Initializes a sstp stream struct for the given socket file descriptor.
 */
SSTPStream *sstp_init(int sockfd);

/*
 * Reads a single message from the socket
 * Returns
 *  1 on success
 *  0 if the client disconnects
 * -1 if an error occurs
 */
int sstp_read(SSTPStream *stream, SSTPMsg *msg);

/*
 * Sends a single message of the given type and payload.
 * Returns
 *  0 on success
 * -1 if an error occurs
 */
int sstp_write(SSTPStream *stream, SSTPMsgType type, char payload[]);

/*
 * Destroys the given sstp stream.
 */
void sstp_destroy(SSTPStream *stream);
