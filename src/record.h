/* SPDX-License-Identifier: MIT */
/*
 * Functions for recording program output.
 *
 * Copyright IBM Corp. 2023
 */

#ifndef RECORD_H
#define RECORD_H

#include <stdbool.h>
#include <stdio.h>
#include <sys/resource.h>
#include <sys/time.h>

/* Data recording scope. */

/* Record output to standard output stream. */
#define REC_STDOUT	1

/* Record output to standard error stream. */
#define REC_STDERR	2

/* Record process resource usage. */
#define REC_RUSAGE	4

/* Record all of the above. */
#define REC_ALL		(REC_STDOUT | REC_STDERR | REC_RUSAGE)

struct rec_result {
	/* Process status as returned by waitpid(). */
	bool status_valid;
	int status;
	bool output_valid;
	/* Stream containing timestamped output. */
	FILE *output;
	/* Number of output bytes recorded. */
	size_t output_size;
	/* Recording start time. */
	struct timeval start_time;
	/* Recording end time. */
	struct timeval stop_time;
	/* Total duration. */
	struct timeval duration;
	/* Resource usage. Note: For inline recording using rec_start() and
	 * rec_stop(), maxrss includes usage before rec_start(). */
	bool rusage_valid;
	struct rusage rusage;
	/* Internal state. */
	void *state;
};

/**
 * struct rec_stream - A single stream to record
 * @name: Name of stream
 * @fd: File descriptor from which to record data
 * @nocount: If set, this stream does not count towards the number of open
 *           streams that must be closed for a recording call to end.
 * @onclose: If set, the handler function is called with a %NULL line when
 *           this stream is closed.
 */
struct rec_stream {
	char *name;
	int fd;
	bool nocount;
	bool onclose;
};

typedef void (*linehandler_t)(void *data, char *line,
			      struct rec_stream *stream);

void rec_log_streams(FILE *log, int streamc, struct rec_stream *streams,
		     linehandler_t handler, void *data,
		     struct timeval *start_time, struct timeval *stop_time);
void rec_record(struct rec_result *res, char *cmd, char *argv[], int scope,
		linehandler_t handler, void *data);
void rec_print(FILE *fd, struct rec_result *res, int indent);
void rec_start(struct rec_result *res, int scope, linehandler_t handler,
	       void *data);
void rec_stop(struct rec_result *res);
void rec_close(struct rec_result *res);
void rec_free_streams(int streamc, struct rec_stream *streams);

#endif /* RECORD_H */
