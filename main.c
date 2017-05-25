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

#include "uint256.h"
#include "log.h"
#include "server.h"
#include "sstp-socket-wrapper.h"
#include "hashcash.h"
#include "queue.h"

#define MAX_LOG_LEN 512

/*
 * The struct that represents a job.
 */
typedef struct {
    // the client that owns this job
    Connection *conn;
    Logger *logger;
    SSTPSocketWrapper *sstp;
    pthread_mutex_t *write_mutex;

    // the WORK information
    SSTPMsg msg; // the original work message
    uint32_t difficulty;
    BYTE target[32];
    BYTE seed[32];
    uint64_t start;
    uint8_t worker_count;
    uint64_t solution;

    // flags
    char abort;
    char solution_found;
} WorkJob;

// the global work queue and active job
Queue *work_queue;
WorkJob *active_job;


/***** Helper function prototypes
 */

// Main functions
void *work_consumer(void *_);
void *client_handler(void *pconn);
void handler_thread_spawner(Connection conn);

// WORK helper functions
void work_parse(char *msg, uint32_t *difficulty, BYTE *seed, uint64_t *start,
        uint8_t *worker_count);
void work_enqueue(Connection *conn, pthread_mutex_t *write_mutex,
        SSTPSocketWrapper *sstp, Logger *logger, SSTPMsg msg);

// SOLN helper functions
void soln_parse(char *msg, uint32_t *difficulty, BYTE *seed, uint64_t *solution);
int soln_verify(char *soln_msg);

// SSTP logging helper functions
void sstp_log(Logger *logger, char *prefix, SSTPMsgType type, char *payload);
int sstp_log_read(SSTPSocketWrapper *sstp, Logger *logger, SSTPMsg *msg);
int sstp_log_write(pthread_mutex_t *write_mutex, SSTPSocketWrapper *sstp,
        Logger *logger, SSTPMsgType type, char payload[]);


/***** Main functions
 */

int main(int argc, char *argv[]) {
    int port;

    if (argc < 2) {
        fprintf(stderr, "ERROR: no port provided\n");
        exit(1);
    }

    port = atoi(argv[1]);

    log_global_init();

    // create the work queue and consumer
    work_queue = queue_init();
    pthread_t tid;
    pthread_create(&tid, NULL, work_consumer, NULL);

    server(port, handler_thread_spawner);

    return 0;
}

/*
 * Handles work jobs that land in the work queue.
 */
void *work_consumer(void *_) {
    (void)_; // purposefully unused, so silence the compiler

    while (1) {
        active_job = queue_dequeue(work_queue);

        // solve the work
        uint64_t nonce = active_job->start;
        while (!hashcash_verify(active_job->target, active_job->seed, nonce)) {
            nonce++;
        }
        active_job->solution_found = 1;
        active_job->solution = nonce;

        // found the solution so send it to the client
        sprintf(active_job->msg.payload + 8 + 1 + 64 + 1,
                "%016" PRIx64, active_job->solution);
        sstp_log_write(active_job->write_mutex, active_job->sstp,
                active_job->logger, SOLN, active_job->msg.payload);

        free(active_job);
        active_job = NULL;
    }

    return NULL;
}

/*
 * Handles communication with each individual client (ie each connection).
 */
void *client_handler(void *pconn) {
    Connection conn = *((Connection *) pconn);
    free(pconn);

    Logger *logger = log_init(conn);
    SSTPSocketWrapper *sstp = sstp_init(conn.sockfd);
    pthread_mutex_t write_mutex = PTHREAD_MUTEX_INITIALIZER;

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
                sstp_log_write(&write_mutex, sstp, logger, PONG, NULL);
                break;
            case PONG:
                sstp_log_write(&write_mutex, sstp, logger, ERRO,
                        "PONG msgs are reserved for the server.");
                break;
            case OKAY:
                sstp_log_write(&write_mutex, sstp, logger, ERRO,
                        "OKAY msgs are reserved for the server.");
                break;
            case ERRO:
                sstp_log_write(&write_mutex, sstp, logger, ERRO,
                        "ERRO msgs are reserved for the server.");
                break;
            case SOLN:
                if (soln_verify(msg.payload)) {
                    sstp_log_write(&write_mutex, sstp, logger, OKAY, NULL);
                } else {
                    sstp_log_write(&write_mutex, sstp, logger, ERRO,
                        "Not a valid solution.");
                }
                break;
            case WORK:
                work_enqueue(&conn, &write_mutex, sstp, logger, msg);
                break;
            default:
                sstp_log_write(&write_mutex, sstp, logger, ERRO,
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
    pthread_create(&tid, &attr, client_handler, (void *) pconn);

    // clean up
    pthread_attr_destroy(&attr);
}


/***** Helper functions
 */

/******** WORK msg helper functions
 */

/*
 * Parse the given WORK message.
 */
void work_parse(char *msg, uint32_t *difficulty, BYTE *seed, uint64_t *start,
        uint8_t *worker_count) {
    // read everything but the worker count
    soln_parse(msg, difficulty, seed, start);
    msg += 8 + 1 + 64 + 1 + 16 + 1;

    // read worker count
    *worker_count = strtoul(msg, NULL, 16);
}

/*
 * Queue the given WORK msg in the work queue.
 */
void work_enqueue(Connection *conn, pthread_mutex_t *write_mutex,
        SSTPSocketWrapper *sstp, Logger *logger, SSTPMsg msg) {
    WorkJob *job = (WorkJob *) malloc(sizeof(WorkJob));
    assert(job);

    job->conn = conn;
    job->logger = logger;
    job->sstp = sstp;
    job->write_mutex = write_mutex;

    job->msg = msg;

    work_parse(msg.payload, &job->difficulty, job->seed, &job->start, &job->worker_count);
    hashcash_calc_target(job->target, job->difficulty);

    job->solution = 0;
    job->abort = 0;
    job->solution_found = 0;

    queue_enqueue(work_queue, job);
}


/******** SOLN msg helper functions
 */

/*
 * Parse the given SOLN message.
 */
void soln_parse(char *msg, uint32_t *difficulty, BYTE *seed, uint64_t *solution) {
    // read difficulty
    sscanf(msg, "%" PRIx32, difficulty);
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
    sscanf(msg, "%" PRIx64, solution);
}

/*
 * Verify the given SOLN message.
 * Returns 1 if it is valid and 0 otherwise.
 */
int soln_verify(char *soln_msg) {
    uint32_t difficulty;
    BYTE seed[32];
    uint64_t solution;

    soln_parse(soln_msg, &difficulty, seed, &solution);

    BYTE target[32];

    hashcash_calc_target(target, difficulty);

    return hashcash_verify(target, seed, solution);
}


/******** SSTP logging helper functions
 */

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
int sstp_log_read(SSTPSocketWrapper *sstp, Logger *logger, SSTPMsg *msg) {
    int res = sstp_read(sstp, msg);
    if (res > 0) { // log only if successful
        sstp_log(logger, "Recieved: ", msg->type, msg->payload);
    }
    return res;
}

/*
 * Wrapper around sstp_write that logs the call.
 */
int sstp_log_write(pthread_mutex_t *write_mutex, SSTPSocketWrapper *sstp,
        Logger *logger, SSTPMsgType type, char payload[]) {

    pthread_mutex_lock(write_mutex);

    int res = sstp_write(sstp, type, payload);
    if (res == 0) { // log only if successful
        sstp_log(logger, "Sending:  ", type, payload);
    }

    pthread_mutex_unlock(write_mutex);

    return res;
}
