/*
 * COMP30023 Computer Systems Project 2
 * Ibrahim Athir Saleem (isaleem) (682989)
 *
 * The module that provides functions for reading from and writing to a network
 * socket using the Simple Stratum Text Protocol (SSTP).
 *
 */

#pragma once

#define ERRO_PAYLOAD_LEN 40
#define SOLN_PAYLOAD_LEN 90 // 8 + 1 + 64 + 1 + 16
#define WORK_PAYLOAD_LEN 93 // 8 + 1 + 64 + 1 + 16 + 1 + 2
#define MAX_PAYLOAD_LEN WORK_PAYLOAD_LEN

/*
 * All the message types.
 */
typedef enum {
    PING,
    PONG,
    OKAY,
    ERRO,
    SOLN,
    WORK,
    ABRT,
    UNRECOGNIZED
} SSTPMsgType;

/*
 * The struct to store the state of the socket.
 * Internals are private.
 */
typedef struct SSTPStream SSTPStream;

/*
 * The struct that represents a message
 */
typedef struct {
    SSTPMsgType type;
    char payload[MAX_PAYLOAD_LEN];
} SSTPMsg;

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
