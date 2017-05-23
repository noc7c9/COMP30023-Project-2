/*
 * COMP30023 Computer Systems Project 2
 * Ibrahim Athir Saleem (isaleem) (682989)
 *
 * Please see the corresponding header file for documentation on the module.
 *
 */

#include <string.h>

#include "sstp.h"

/*
 * Simple helper function to figure out the message type given the header as a
 * string.
 */
SSTPMsgType header_to_type(char *header) {
    if (0 == strncmp("PING", header, HEADER_LEN)) {
        return PING;
    } else if (0 == strncmp("PONG", header, HEADER_LEN)) {
        return PONG;
    } else if (0 == strncmp("OKAY", header, HEADER_LEN)) {
        return OKAY;
    } else if (0 == strncmp("ERRO", header, HEADER_LEN)) {
        return ERRO;
    } else if (0 == strncmp("SOLN", header, HEADER_LEN)) {
        return SOLN;
    } else if (0 == strncmp("WORK", header, HEADER_LEN)) {
        return WORK;
    } else if (0 == strncmp("ABRT", header, HEADER_LEN)) {
        return ABRT;
    } else {
        return UNRECOGNIZED;
    }
}


/*
 * A simple helper function to get the (fixed) payload length for the given
 * message type.
 */
int type_to_payload_len(SSTPMsgType type) {
    switch (type) {
        case ERRO: return ERRO_PAYLOAD_LEN;
        case SOLN: return SOLN_PAYLOAD_LEN;
        case WORK: return WORK_PAYLOAD_LEN;
        default:   return 0;
    }
}


void sstp_parse(char *src, int n, SSTPMsg *msg) {
    // start by clearing out the msg object
    memset(msg, 0, sizeof(SSTPMsg));

    msg->type = header_to_type(src);
    msg->payload_len = type_to_payload_len(msg->type);

    // handle messages that are the wrong length
    int invalid_length = msg->payload_len == 0
        // payload when there shouldn't be any
        ? (HEADER_LEN + DELIMITER_LEN) != n
        // if there is a payload it should be exactly the right length
        : (HEADER_LEN + 1 + msg->payload_len + DELIMITER_LEN) != n;
    if (invalid_length) {
        msg->type = UNRECOGNIZED;
        msg->payload_len = 0;
        return;
    }

    // copy the payload (if any)
    if (msg->payload_len != 0) {
        memcpy(msg->payload, src + HEADER_LEN + 1, msg->payload_len);
    }
}

/*
 * Helper to write the proper header based on the given type to the given
 * destination string.
 */
void copy_header(SSTPMsgType type, char *dst) {
    switch (type) {
        case PING: strncpy(dst, "PING", HEADER_LEN); break;
        case PONG: strncpy(dst, "PONG", HEADER_LEN); break;
        case OKAY: strncpy(dst, "OKAY", HEADER_LEN); break;
        case ERRO: strncpy(dst, "ERRO", HEADER_LEN); break;
        case SOLN: strncpy(dst, "SOLN", HEADER_LEN); break;
        case WORK: strncpy(dst, "WORK", HEADER_LEN); break;
        case ABRT: strncpy(dst, "ABRT", HEADER_LEN); break;
        case UNRECOGNIZED: break; // invalid so do nothing
    }
}

/*
 * Integer min function
 */
int min(int a, int b) {
    return (a > b) ? b : a;
}

int sstp_build(SSTPMsg *msg, char *dst) {
    if (msg->type == UNRECOGNIZED) return -1;

    int length = 0;

    // make sure the payload length is correct
    int actual_payload_len = msg->payload_len;
    msg->payload_len = type_to_payload_len(msg->type);

    // add the header
    copy_header(msg->type, dst);
    dst += HEADER_LEN;
    length += HEADER_LEN;

    // add the payload if any
    if (msg->payload_len > 0) {
        // space between header and payload
        *dst = ' ';
        dst += 1;
        length += 1;

        // the payload itself
        // padding/truncating as necessary
        memset(dst, '\0', msg->payload_len);
        memcpy(dst, msg->payload, min(msg->payload_len, actual_payload_len));
        dst += msg->payload_len;
        length += msg->payload_len;
    }

    // add the delimiter (and the null terminator)
    strncpy(dst, DELIMITER, DELIMITER_LEN + 1);
    length += DELIMITER_LEN;

    return length;
}
