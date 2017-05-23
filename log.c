/*
 * COMP30023 Computer Systems Project 2
 * Ibrahim Athir Saleem (isaleem) (682989)
 *
 * Please see the corresponding header file for documentation on the module.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <pthread.h>

#include "server.h"

#include "log.h"

#define LOG_FILE_PATH "log.txt"
#define MAX_IP_LEN 4 + 4 + 4 + 3
#define HEADER_LEN 100 // should be enough

#define TIME_FORMAT "%Y-%m-%d %H:%M:%S"
#define TIME_LEN 19

// global file descriptor and mutex
FILE *log_fd = NULL;
pthread_mutex_t mutex;

struct Logger {
    char header[HEADER_LEN];
};

void log_global_init() {
    if (log_fd != NULL) { return; }
    log_fd = fopen(LOG_FILE_PATH, "w");

    pthread_mutex_init(&mutex, NULL);
}

Logger *log_init(Connection conn) {
    Logger *logger = malloc(sizeof(Logger));
    assert(NULL != logger);

    sprintf(logger->header, "[ %s (%d) %%s ] ", conn.ip, conn.sockfd);

    return logger;
}

void get_time_str(char *dst, int maxlen) {
    time_t rawtime;
    struct tm *timeinfo;

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    strftime(dst, maxlen, TIME_FORMAT, timeinfo);
}

void log_print(Logger *logger, char *msg) {
    char datetime[TIME_LEN + 1];
    get_time_str(datetime, TIME_LEN + 1);

    printf(logger->header, datetime);
    printf(msg);
    printf("\n");

    pthread_mutex_lock(&mutex);
    fprintf(log_fd, logger->header, datetime);
    fprintf(log_fd, msg);
    fprintf(log_fd, "\n");
    fflush(log_fd);
    pthread_mutex_unlock(&mutex);
}

void log_destroy(Logger *logger) {
    free(logger);
}
