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
#include <inttypes.h>

#include "log.h"
#include "server.h"
#include "sstp-socket-wrapper.h"

#include "uint256.h"
#include "sha256.h"

#define MAX_LOG_LEN 512


/*
 * Initializes the given uint256 values with the given initial value.
 */
void uint256_init_with_value(BYTE *uint256, uint64_t value) {
    if (uint256 == NULL) {
        return;
    }

    uint256_init(uint256);

    for (int i = 31; i >= 0 && value > 0; i--) {
        uint256[i] = value & 0xff;
        value = value >> 8;
    }
}


/*
 * Converts the given difficulty value into the target value. (32-byte array)
 */
void difficulty_to_target(BYTE *target, uint32_t difficulty) {
    BYTE alpha[32];
    BYTE beta[32];
    BYTE two[32];

    uint256_init_with_value(beta, difficulty & 0xffffff);

    uint256_init_with_value(two, 2);
    uint256_exp(alpha, two, 8 * ((difficulty >> 24) - 3));

    uint256_mul(target, beta, alpha);
}


/*
 * Hashes the given value using sha256 twice.
 */
void sha256twice(BYTE *hash, BYTE *data, int len) {
    SHA256_CTX ctx;

    sha256_init(&ctx);
    sha256_update(&ctx, data, len);
    sha256_final(&ctx, hash);

    sha256_init(&ctx);
    sha256_update(&ctx, hash, 32);
    sha256_final(&ctx, hash);
}


/*
 * Concat the given seed and nonce value.
 * Assumes seed is a BYTE array with enough space for the concatenation.
 */
void concat(BYTE *seed, uint64_t nonce) {
    seed += 32;
    for (int i = 7; i >= 0 && nonce > 0; i--) {
        seed[i] = nonce & 0xff;
        nonce = nonce >> 8;
    }
}


/*
 * Parse the given SOLN message.
 */
void soln_parse(char *msg, uint32_t *difficulty, BYTE *seed, uint64_t *solution) {
    // read difficulty
    sscanf(msg, "%x", difficulty);
    msg += 8 + 1;

    // read seed
    char copy[65]; // create a copy as the string needs to be modified
    strncpy(copy, msg, 64);
    copy[64] = '\0';
    // the actual reading
    int j = 32;
    for (int i = 62; i >= 0; i -= 2) {
        copy[i+2] = '\0';
        seed[--j] = strtoul(copy + i, NULL, 16);
    }
    msg += 64 + 1;

    // read solution
    sscanf(msg, "%lx", solution);
}


/*
 * Verify the given SOLN message.
 * Returns 1 if it is valid and 0 otherwise.
 */
int soln_verify(char *soln_msg) {
    uint32_t difficulty;
    BYTE seed[40]; // accommodate the concatenated solution
    uint64_t solution;
    BYTE target[32];
    BYTE hash[32];

    // parse
    soln_parse(soln_msg, &difficulty, seed, &solution);

    // concat
    concat(seed, solution);
    sha256twice(hash, seed, 40);

    // calculate target
    difficulty_to_target(target, difficulty);

    // compare
    return -1 == sha256_compare(hash, target);
}


/*
 * Helper to log sstp messages.
 */
void sstp_log(Logger *logger, char *prefix, SSTPMsgType type, char *payload) {
    char *header;
    switch (type) {
        case PING: header = "PING"; break;
        case PONG: header = "PONG"; break;
        case OKAY: header = "OKAY"; break;
        case ERRO: header = "ERRO"; break;
        case SOLN: header = "SOLN"; break;
        case WORK: header = "WORK"; break;
        case ABRT: header = "ABRT"; break;
        default:   header = "Malformed Message"; // invalid so do nothing
    }

    // create the message
    char buf[MAX_LOG_LEN];
    switch (type) {
        // no payload
        case PING:
        case PONG:
        case OKAY:
        case ABRT:
        case MALFORMED:
            snprintf(buf, MAX_LOG_LEN, "%s%s",
                    prefix, header);
            break;
        // with payload
        case SOLN:
        case WORK:
        case ERRO:
            snprintf(buf, MAX_LOG_LEN, "%s%s %s",
                    prefix, header, payload);
            break;
    }
    log_print(logger, buf);
}


/*
 * Wrapper around sstp_read that logs the call.
 */
int sstp_log_read(SSTPStream *sstp, Logger *logger, SSTPMsg *msg) {
    int res = sstp_read(sstp, msg);
    if (res > 0) { // log only if successful
        sstp_log(logger, "Recieved: ", msg->type, msg->payload);
    }
    return res;
}


/*
 * Wrapper around sstp_write that logs the call.
 */
int sstp_log_write(SSTPStream *sstp, Logger *logger,
        SSTPMsgType type, char payload[]) {
    int res = sstp_write(sstp, type, payload);
    if (res == 0) { // log only if successful
        sstp_log(logger, "Sending:  ", type, payload);
    }
    return res;
}


/*
 * Handles communication with each individual client (ie each connection).
 */
void *client_handler(void *pconn) {
    Connection conn = *((Connection *) pconn);
    free(pconn);

    Logger *logger = log_init(conn);
    SSTPStream *sstp = sstp_init(conn.sockfd);

    log_print(logger, "Connected");

    SSTPMsg msg;

    int res;
    while (0 != (res = sstp_log_read(sstp, logger, &msg))) {
        if (res < 0) {
            perror("ERROR: reading from socket");
            return NULL;
        }

        switch (msg.type) {
            case PING:
                sstp_log_write(sstp, logger, PONG, NULL);
                break;
            case PONG:
                sstp_log_write(sstp, logger, ERRO,
                        "PONG msgs are reserved for the server.");
                break;
            case OKAY:
                sstp_log_write(sstp, logger, ERRO,
                        "OKAY msgs are reserved for the server.");
                break;
            case ERRO:
                sstp_log_write(sstp, logger, ERRO,
                        "ERRO msgs are reserved for the server.");
                break;
            default:
                sstp_log_write(sstp, logger, ERRO,
                        "Malformed message.");
                break;
        }
    }

    log_print(logger, "Disconnected");

    // clean up
    sstp_destroy(sstp);
    log_destroy(logger);
    close(conn.sockfd);

    return NULL;
}


/*
 * A server handler function that spawns a detached thread to actually handle
 * each connection.
 */
void handler_thread_spawner(Connection conn) {
    // alloc to passing the connection to the thread
    Connection *pconn = malloc(sizeof(Connection));
    assert(NULL != pconn);
    *pconn = conn;

    // threads should be created detached, as they don't return anything
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    // create the thread
    pthread_t tid;
    pthread_create(&tid, NULL, client_handler, (void *) pconn);

    // clean up
    pthread_attr_destroy(&attr);
}


int main(int argc, char *argv[]) {
    // int port;

    // if (argc < 2) {
    //     fprintf(stderr, "ERROR: no port provided\n");
    //     exit(1);
    // }

    // port = atoi(argv[1]);

    // log_global_init();

    // server(port, handler_thread_spawner);


    printf("res: %d\n", soln_verify("1fffffff 0000000019d6689c085ae165831e934ff763ae46a218a6c172b3f1b60a8ce26f 1000000023212147"));
    printf("res: %d\n", soln_verify("1effffff 0000000019d6689c085ae165831e934ff763ae46a218a6c172b3f1b60a8ce26f 100000002321ed8f"));
    printf("res: %d\n", soln_verify("1fffffff 0000000019d6689c085ae165831e934ff763ae46a218a6c172b3f1b60a8ce26f 1000000023212605"));


    return 0;
}
