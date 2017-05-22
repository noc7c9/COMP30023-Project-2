/*
 * COMP30023 Computer Systems Project 2
 * Ibrahim Athir Saleem (isaleem) (682989)
 *
 * Please see the corresponding header file for documentation on the module.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <sys/socket.h>

#include "sstp.h"

#define DELIMITER "\r\n"
#define DELIMITER_LEN 2
#define HEADER_LEN 4
#define MAX_MSG_LEN HEADER_LEN + 1 + MAX_PAYLOAD_LEN + DELIMITER_LEN

#define BUFLEN 256 // TODO: choose a smarter value

struct SSTPStream {
    int sockfd;
};

SSTPStream *sstp_init(int sockfd) {
    SSTPStream *stream = malloc(sizeof(SSTPStream));
    assert(NULL != stream);

    stream->sockfd = sockfd;

    return stream;
}

int sstp_read(SSTPStream *stream, SSTPMsg *msg) {
    char header[HEADER_LEN];
    int read_n;

    // TODO: for now a naive implementation that assumes sockets just work
    read_n = recv(stream->sockfd, header, HEADER_LEN, 0);
    if (read_n <= 0) {
        return read_n;
    }
    header[read_n] = '\0';

    // figure out which message type it is
    printf("RECV[%s]\n", header);
    if (0 == strncmp("PING", header, HEADER_LEN)) {
        msg->type = PING;

    } else if (0 == strncmp("PONG", header, HEADER_LEN)) {
        msg->type = PONG;

    } else if (0 == strncmp("OKAY", header, HEADER_LEN)) {
        msg->type = OKAY;

    } else if (0 == strncmp("ERRO", header, HEADER_LEN)) {
        msg->type = ERRO;

    } else if (0 == strncmp("SOLN", header, HEADER_LEN)) {
        msg->type = SOLN;

    } else if (0 == strncmp("WORK", header, HEADER_LEN)) {
        msg->type = WORK;

    } else if (0 == strncmp("ABRT", header, HEADER_LEN)) {
        msg->type = ABRT;

    } else {
        msg->type = UNRECOGNIZED;
    }

    return 1; // success
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

/*
 * Helper function that sends a message that has no payload
 */
int sstp_write_no_payload(SSTPStream *stream, char header[]) {
    int len = HEADER_LEN + DELIMITER_LEN;
    char buf[HEADER_LEN + DELIMITER_LEN];

    strncpy(buf, header, HEADER_LEN);
    strncpy(buf+HEADER_LEN, DELIMITER, DELIMITER_LEN);

    return sendall(stream->sockfd, buf, &len);
}

/*
 * Integer min function
 */
int min(int a, int b) {
    return (a > b) ? b : a;
}

/*
 * Helper function that sends a message along with its payload
 */
int sstp_write_with_payload(SSTPStream *stream, char header[],
        char payload[], int fixed_payload_len) {
    char buf[MAX_MSG_LEN];
    bzero(buf, MAX_MSG_LEN);

    // build up the message
    char *pbuf = buf;

    // add the header
    strncpy(pbuf, header, HEADER_LEN);
    pbuf += HEADER_LEN;

    // add the space between header and payload
    *pbuf = ' ';
    pbuf += 1;

    // add the payload
    strncpy(pbuf, payload, min(strlen(payload), fixed_payload_len));
    pbuf += fixed_payload_len; // skip over the unwritten payload section

    // add the delimiter
    strncpy(pbuf, DELIMITER, DELIMITER_LEN);

    // send the message
    int len = HEADER_LEN + 1 + fixed_payload_len + DELIMITER_LEN;
    return sendall(stream->sockfd, buf, &len);
}

int sstp_write(SSTPStream *stream, SSTPMsgType type, char payload[]) {
    int payload_len;
    switch (type) {
        case ERRO:
            payload_len = ERRO_PAYLOAD_LEN;
            break;
        case SOLN:
            payload_len = SOLN_PAYLOAD_LEN;
            break;
        case WORK:
            payload_len = WORK_PAYLOAD_LEN;
            break;
        default: // no payload
            payload_len = 0;
    }

    switch (type) {
        // messages without payload
        case PING: return sstp_write_no_payload(stream, "PING");
        case PONG: return sstp_write_no_payload(stream, "PONG");
        case OKAY: return sstp_write_no_payload(stream, "OKAY");
        case ABRT: return sstp_write_no_payload(stream, "ABRT");

        // messages with payload
        case ERRO:
            return sstp_write_with_payload(stream, "ERRO", payload, payload_len);
        case SOLN:
            return sstp_write_with_payload(stream, "SOLN", payload, payload_len);
        case WORK:
            return sstp_write_with_payload(stream, "WORK", payload, payload_len);
        default:
            return -1; // unrecognized type
    }
}

void sstp_destroy(SSTPStream *stream) {
    free(stream);
}
