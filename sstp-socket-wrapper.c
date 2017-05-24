/*
 * COMP30023 Computer Systems Project 2
 * Ibrahim Athir Saleem (isaleem) (682989)
 *
 * Please see the corresponding header file for documentation on the module.
 *
 */

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <sys/socket.h>

#include "sstp.h"
#include "sstp-socket-wrapper.h"

#define DELIMITER "\r\n"
#define DELIMITER_LEN 2
#define HEADER_LEN 4
#define MAX_MSG_LEN HEADER_LEN + 1 + MAX_PAYLOAD_LEN + DELIMITER_LEN


/***** Private structs
 */

struct SSTPStream {
    int sockfd;
    char buffer[MAX_MSG_LEN + 1];
    int buffer_len;
};


/***** Helper function prototypes
 */

char *strnstr(char *haystack, char *needle, int n);
int sendall(int s, char *buf, int *len);


/***** Public functions
 */

SSTPStream *sstp_init(int sockfd) {
    SSTPStream *stream = malloc(sizeof(SSTPStream));
    assert(NULL != stream);

    stream->sockfd = sockfd;
    stream->buffer_len = 0;

    return stream;
}

int sstp_read(SSTPStream *stream, SSTPMsg *msg) {
    char buffer[MAX_MSG_LEN + 1];
    int buffer_len = 0;
    int overflow = 0;
    char *match = NULL;
    int read_n;

    // populate buffer with the previous call's buffer contents
    if (stream->buffer_len > 0) {
        strncpy(buffer, stream->buffer, stream->buffer_len);
        buffer_len = stream->buffer_len;
        stream->buffer_len = 0;
    } else { // otherwise, buffer should start as an empty string
        buffer[0] = '\0';
    }

    // read until hitting a delimiter
    while (1) {
        // look for the delimiter
        match = strnstr(buffer, DELIMITER, buffer_len);
        if (match != NULL) { // found it!
            // include the delimiter in the match
            match += DELIMITER_LEN;

            // keep the rest of the buffer for next call
            strcpy(stream->buffer, match);
            stream->buffer_len = buffer_len - (match - buffer);
            buffer_len = match - buffer;

            // terminate the match
            buffer[match - buffer] = '\0';

            // stop reading and start processing the match
            break;
        }

        // if buffer is completely filled up without a delimiter, then the
        // message is too long
        if (buffer_len == MAX_MSG_LEN) {
            // so reset the buffer, but mark the message as having overflowed
            overflow += buffer_len;
            buffer_len = 0;
        }

        // read in more data from the socket
        read_n = recv(stream->sockfd, buffer + buffer_len, MAX_MSG_LEN - buffer_len, 0);
        buffer_len += read_n;

        buffer[buffer_len] = '\0';

        // stop if an error occurs
        if (read_n <= 0) {
            return read_n;
        }
    }

    // parse the read data into an SSTPMsg
    sstp_parse(buffer, buffer_len + overflow, msg);

    return 1; // success
}

int sstp_write(SSTPStream *stream, SSTPMsgType type, char payload[]) {
    // create the message
    SSTPMsg msg;
    msg.type = type;

    if (payload != NULL) {
        msg.payload_len = strlen(payload);
        memcpy(msg.payload, payload, msg.payload_len);
    }

    // build the message string
    char buf[MAX_MSG_LEN+1];
    int len = sstp_build(&msg, buf);

    return sendall(stream->sockfd, buf, &len);
}

void sstp_destroy(SSTPStream *stream) {
    free(stream);
}


/***** Helper functions
 */

/*
 * Alternative to strstr that ignores \0 characters in the haystack and instead
 * scans upto n characters.
 * needle should still be a null-terminated string.
 */
char *strnstr(char *haystack, char *needle, int n) {
    char *p = haystack;
    int needle_len = strlen(needle);
    n -= needle_len - 1; // don't scan off the edge

    while (p - haystack < n) {
        if (0 == strncmp(p, needle, needle_len)) {
            return p;
        }
        p++;
    }

    return NULL; // not found
}

/*
 * Helper that makes sure that partial sends don't happen (if it can be helped).
 * source: https://beej.us/guide/bgnet/output/html/multipage/advanced.html#sendall
 */
int sendall(int s, char *buf, int *len) {
    int total = 0;        // how many bytes we've sent
    int bytesleft = *len; // how many we have left to send
    int n;

    while (total < *len) {
        n = send(s, buf+total, bytesleft, 0);
        if (n == -1) { break; }
        total += n;
        bytesleft -= n;
    }

    *len = total; // return number actually sent here

    return n == -1 ? -1 : 0; // return -1 on failure, 0 on success
}
