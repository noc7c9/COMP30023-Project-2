/*
 * COMP30023 Computer Systems Project 2
 * Ibrahim Athir Saleem (isaleem) (682989)
 *
 * The module that provides functions for parsing and building strings formatted
 * using the Simple Stratum Text Protocol (SSTP).
 *
 */

#pragma once

#define HEADER_LEN 4

#define ERRO_PAYLOAD_LEN 40
#define SOLN_PAYLOAD_LEN 90 // 8 + 1 + 64 + 1 + 16
#define WORK_PAYLOAD_LEN 93 // 8 + 1 + 64 + 1 + 16 + 1 + 2
#define MAX_PAYLOAD_LEN WORK_PAYLOAD_LEN

#define DELIMITER "\r\n"
#define DELIMITER_LEN 2

#define MAX_MSG_LEN HEADER_LEN + 1 + MAX_PAYLOAD_LEN + DELIMITER_LEN

/*
 * All the message types.
 */
typedef enum {
    PING, PONG,
    OKAY, ERRO,
    SOLN, WORK, ABRT,
    MALFORMED
} SSTPMsgType;

/*
 * The struct that represents a message
 */
typedef struct {
    SSTPMsgType type;
    char payload[MAX_PAYLOAD_LEN + 1];
    int payload_len;
} SSTPMsg;

/*
 * Parses the given source string into the given SSTPMsg.
 * n is the length of src.
 */
void sstp_parse(char *src, int n, SSTPMsg *msg);

/*
 * Creates a SSTP formatted string from the given message
 * and puts it into the destination string,
 * destination string must be big enough to store the longest possible message
 * plus the null terminator.
 *
 * Returns the length of the final message or -1 if there is an error.
 */
int sstp_build(SSTPMsg *msg, char *dst);
