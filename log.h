/*
 * COMP30023 Computer Systems Project 2
 * Ibrahim Athir Saleem (isaleem) (682989)
 *
 * The module that provides thread-safe per-client logging functionality.
 *
 */

#pragma once

#include "server.h"

/*
 * The struct that represents a logger
 * primarily stores the details about a client.
 * Internals are private.
 */
typedef struct Logger Logger;

/*
 * Does the global initialization.
 */
void log_global_init();

/*
 * Initializes a Logger struct
 */
Logger *log_init(Connection conn);

/*
 * Logs out the given message.
 */
void log_print(Logger *logger, char *msg);

/*
 * Destroys the Logger.
 */
void log_destroy(Logger *logger);
