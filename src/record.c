/* SPDX-License-Identifier: MIT */
/*
 * Functions for recording program output.
 *
 * Copyright IBM Corp. 2023
 */

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "misc.h"
#include "record.h"

/* Internal monitoring state used to manage monitoring-related sub-process. */
struct rec_mon {
	bool source;
	int scope;
	pid_t pid;
	int orig_stdout;
	int orig_stderr;
	int stdout_p[2];
	int stderr_p[2];
	linehandler_t handler;
	void *data;
	FILE *log;
};

/* Initialize monitoring data structure @mon. */
static void rec_mon_init(struct rec_mon *mon, int scope, linehandler_t handler,
			 void *data)
{
	memset(mon, 0, sizeof(*mon));

	if (pipe(mon->stdout_p) == -1 || pipe(mon->stderr_p) == -1)
		err(1, "Could not create pipes");

	mon->scope = scope;
	mon->handler = handler;
	mon->data = data;
	mon->log = tmpfile();
	if (!mon->log)
		err(1, "Could not create temporary file");

	/* There's no need to keep these fds open in exec'd processes. */
	misc_cloexec(mon->stdout_p[0]);
	misc_cloexec(mon->stdout_p[1]);
	misc_cloexec(mon->stderr_p[0]);
	misc_cloexec(mon->stderr_p[1]);
	misc_cloexec(fileno(mon->log));
}

/* Prepare monitoring data structure @mon. If @source is true, the calling
 * process will provide data to record, otherwise the process will record
 * data. */
static void rec_mon_prepare(struct rec_mon *mon, bool source)
{
	mon->source = source;
	if (source) {
		/* This process will run the command to record. */
		close(mon->stdout_p[PREAD]);
		close(mon->stderr_p[PREAD]);

		if ((mon->orig_stdout = dup(STDOUT_FILENO)) == -1 ||
		    (mon->orig_stderr = dup(STDERR_FILENO)) == -1)
			err(1, "Could not duplicate output streams");

		misc_cloexec(mon->orig_stdout);
		misc_cloexec(mon->orig_stderr);
	} else {
		/* This process will monitor the other process. */
		close(mon->stdout_p[PWRITE]);
		close(mon->stderr_p[PWRITE]);
	}
}

/* Release resources associated with @mon. */
static void rec_mon_cleanup(struct rec_mon *mon)
{
	if (mon->source) {
		close(mon->stdout_p[PWRITE]);
		close(mon->stderr_p[PWRITE]);
		close(mon->orig_stdout);
		close(mon->orig_stderr);
	} else {
		close(mon->stdout_p[PREAD]);
		close(mon->stderr_p[PREAD]);
	}
	fflush(mon->log);
}

/* Redirect stdout and stderr to @new_stdout and @new_stderr. */
static void rec_redirect(int scope, int new_stdout, int new_stderr)
{
	fflush(stdout);
	fflush(stderr);

	if (scope & REC_STDOUT) {
		close(STDOUT_FILENO);
		if (dup2(new_stdout, STDOUT_FILENO) == -1)
			err(1, "Could not redirect standard output");
	}

	if (scope & REC_STDERR) {
		close(STDERR_FILENO);
		if (dup2(new_stderr, STDERR_FILENO) == -1)
			err(1, "Could not redirect standard error");
	}
}

/* Run command specified by @cmd and @argv while redirecting output according
 * to @mon. */
static void rec_child(struct rec_mon *mon, char *cmd, char *argv[])
{
	int e;

	/* Child: Run command.  */
	rec_mon_prepare(mon, true);
	rec_redirect(mon->scope, mon->stdout_p[PWRITE], mon->stderr_p[PWRITE]);
	execv(cmd, argv);

	/* execv() failed - save errno for err() call. */
	e = errno;
	rec_redirect(mon->scope, mon->orig_stdout, mon->orig_stderr);
	errno = e;
	err(1, "Could not run command '%s'", cmd);
}

static void do_log_buf(FILE *log, struct timeval *tv, char *name, char *buf,
		       size_t len)
{
	bool nl = buf[len - 1] == '\n';

	/* Write line header. */
	fprintf(log, "[%4lu.%06lu] %s%s: ", (unsigned long) tv->tv_sec,
		(unsigned long) tv->tv_usec, name, nl ? "" : "(nonl)");

	/* Write line contents. */
	fwrite(buf, 1, len, log);
	if (!nl)
		fprintf(log, "\n");
}

static void handle_line(FILE *log, struct rec_stream *stream,
			struct timeval *tv, linehandler_t handler, void *data,
			char *buf, size_t len)
{
	char *copy;

	/* Call handler if set. */
	if (handler) {
		copy = strndup(buf, len);
		if (!copy)
			oom();
		handler(data, copy, stream);
		free(copy);
	}

	/* Write to log if set. */
	if (log)
		do_log_buf(log, tv, stream->name,  buf, len);
}

/* Use this structure to preserve data read from a stream but not yet
 * consumed because of a lack of newline character. */
struct stream_state {
	char *buffer;
	ssize_t buflen;
	off_t off;
};

#define BUFLEN	1024

/* Copy data from @stream->fd to @log line-by-line, while prefixing each output
 * line with a header. */
static size_t rec_log_line(FILE *log, struct rec_stream *stream,
			   struct timeval *tv, linehandler_t handler,
			   void *data, struct stream_state *ss)
{
	char *buffer;
	ssize_t rc, total = 0, buflen;
	off_t off, i;
	bool cont = true;
	struct pollfd fds[1];

	/* Initialize local stream state. */
	if (ss && ss->buffer) {
		buflen = ss->buflen;
		buffer = ss->buffer;
		off = ss->off;
	} else {
		buflen = BUFLEN;
		buffer = misc_malloc(buflen);
		off = 0;
	}

	fds[0].fd = stream->fd;
	fds[0].events = POLLIN;
	while (cont &&
	       (rc = read(stream->fd, buffer + off, buflen - off)) > 0) {
		total += rc;
		rc += off;

		/* Check if there is more data to read. */
		cont = poll(fds, 1, 0) == 1 && (fds[0].revents & POLLIN);

		off = 0;
		while (rc > 0) {
			/* Get line end. */
			for (i = 0; i < rc && buffer[i] != '\n'; i++)
				;

			if (i < rc) {
				/* Include newline when writing line. */
				i++;
			} else if (cont || ss) {
				/* Got partial line, read rest. */
				if (rc == buflen) {
					/* Line exceeds buffer size. */
					buflen *= 2;
					buffer = realloc(buffer, buflen);
					if (!buffer)
						oom();
				}

				if (cont) {
					/* More data available. */
					off = rc;
					break;
				}

				/* No new line and no more data - save
				 * line until next time data is available. */
				ss->buffer = buffer;
				ss->buflen = buflen;
				ss->off = rc;
				goto out;
			}

			handle_line(log, stream, tv, handler, data, buffer, i);

			rc -= i;
			if (rc > 0)
				memmove(buffer, buffer + i, rc);
		}
	}

	/* Consume residual data (on EOF). */
	if (off > 0)
		handle_line(log, stream, tv, handler, data, buffer, off);

	free(buffer);

	if (ss) {
		/* All data consumed - clear stream state. */
		ss->buffer = NULL;
	}

out:
	return total;
}

struct ctl_data {
	FILE *log;
	struct timeval *tv;
	struct pollfd **fds_ptr;
	struct rec_stream **streams_ptr;
	int *streamc_ptr;
	int *openfd_ptr;
};

/* Flag used to indicate that logging should end. */
static bool log_stop;

static void log_sig_handler(int signum)
{
	log_stop = true;
}

static void do_log_str(FILE *log, struct timeval *tv, char *name, char *fmt,
		       ...)
{
	get_varargs(fmt, str);

	do_log_buf(log, tv, name, str, strlen(str));
	free(str);
}

/* Handle requests to open new streams sent via control file descriptors.
 * Format of requests must be "<stream name>:<path to stream>". */
static void ctl_handler(void *data, char *line, struct rec_stream *stream)
{
	struct ctl_data *ctl = data;
	struct rec_stream *streams = *ctl->streams_ptr;
	int streamc = *ctl->streamc_ptr, i, fd;
	struct pollfd *fds;
	char *filename;

	misc_strip_space(line);
	filename = strchr(line, ':');
	if (!filename) {
		do_log_str(ctl->log, ctl->tv, line,
			   "Warning: Missing colon in stream argument");
		return;
	}
	*filename = 0;
	filename++;

	/* Check for duplicates. */
	for (i = 0; i < streamc; i++) {
		if (streams[i].name && strcmp(streams[i].name, line) == 0) {
			do_log_str(ctl->log, ctl->tv, line,
				   "Warning: Duplicate stream registered '%s'",
				   line);
			return;
		}
	}

	fd = open(filename, O_RDONLY);
	if (fd == -1) {
		do_log_str(ctl->log, ctl->tv, line,
			   "Could not open file '%s': %s", filename,
			   strerror(errno));
		return;
	}

	/* Add new pollfd object. */
	fds = misc_malloc(sizeof(struct pollfd) * (streamc + 1));
	memcpy(fds, *ctl->fds_ptr, sizeof(struct pollfd) * streamc);
	fds[streamc].fd = fd;
	fds[streamc].events = POLLIN;

	/* Add new stream object. */
	streams = misc_malloc(sizeof(struct rec_stream) * (streamc + 1));
	memcpy(streams, *ctl->streams_ptr, sizeof(struct rec_stream) * streamc);
	streams[streamc].name = misc_strdup(line);
	streams[streamc].fd = fd;
	streamc++;

	/* Update variables of main function. */
	*ctl->fds_ptr = fds;
	*ctl->streams_ptr = streams;
	*ctl->streamc_ptr = streamc;
	(*ctl->openfd_ptr)++;
}

/* Receive output generated on specified file descriptors and store in log
 * format. If specified, call handler for each line. An entry in @streams
 * with a %NULL for name is not logged, but indicates a control stream
 * that can be used to add new streams. */
void rec_log_streams(FILE *log, int streamc, struct rec_stream *streams,
		     linehandler_t handler, void *data,
		     struct timeval *start_time, struct timeval *stop_time)
{
	struct pollfd *fds;
	struct timeval tv;
	int i, openfd;
	bool eof;
	struct ctl_data ctl;
	struct stream_state *ss;

	debug("%s: starting logging\n", __func__);

	fds = misc_malloc(sizeof(struct pollfd) * streamc);
	ss = misc_malloc(sizeof(struct stream_state) * streamc);

	/* Set file descriptors to non-blocking. */
	openfd = 0;
	for (i = 0; i < streamc; i++) {
		fds[i].fd = streams[i].fd;
		fds[i].events = POLLIN;

		/* Count only non-control file descriptors as open. */
		if (streams[i].name && !streams[i].nocount)
			openfd++;
	}

	ctl.log = log;
	ctl.tv = &tv;
	ctl.fds_ptr = &fds;
	ctl.streams_ptr = &streams;
	ctl.streamc_ptr = &streamc;
	ctl.openfd_ptr = &openfd;

	/* Enable stop via SIGUSR1. */
	log_stop = false;
	signal(SIGUSR1, log_sig_handler);

	/* Receive data from streams until all file descriptors are closed. */
	while (openfd > 0 && !log_stop) {
		if (poll(fds, streamc, -1) == -1) {
			if (errno == EINTR)
				continue;
			break;
		}
		debug("%s: poll events received\n", __func__);

		gettimeofday(&tv, NULL);
		if (start_time)
			timersub(&tv, start_time, &tv);

		for (i = 0; i < streamc; i++) {
			debug("%s: poll event: fd=%d/%s events=%04x "
			      "revents=%04x\n", __func__, fds[i].fd,
			      streams[i].name, fds[i].events, fds[i].revents);

			eof = false;
			if (fds[i].revents & POLLIN) {
				if (streams[i].name) {
					/* Stream data. */
					if (rec_log_line(log, &streams[i], &tv,
							 handler, data,
							 &ss[i]) == 0)
						eof = true;
				} else {
					/* Control data. */
					if (rec_log_line(NULL, &streams[i], &tv,
							 ctl_handler,
							 &ctl, NULL) == 0)
						eof = true;
				}
			} else if (fds[i].revents) {
				/* EOF or POLLERR, POLLHUP or POLLNVAL. */
				eof = true;
			}
			if (eof) {
				if (ss[i].buffer) {
					/* Consume pending data (without nl). */
					handle_line(log, &streams[i], &tv,
						    handler, data, ss[i].buffer,
						    ss[i].off);
					free(ss->buffer);
					ss->buffer = NULL;

				}

				/* Send closing event if requested. */
				if (handler && streams[i].onclose)
					handler(data, NULL, &streams[i]);

				/* Stop watching this file descriptor. */
				fds[i].fd = -1;
				if (streams[i].name && !streams[i].nocount)
					openfd--;

				debug("%s: poll close event: fd %d/%s closed, "
				      "%d remaining\n", __func__, fds[i].fd,
				      streams[i].name, openfd);
			}
		}
	}
	if (stop_time)
		timeradd(&tv, start_time, stop_time);

	free(ss);
	free(fds);

	debug("%s: ending logging\n", __func__);
}

static void rec_log(struct rec_mon *mon, struct timeval *start_time,
		    struct timeval *stop_time)
{
	struct rec_stream streams[2];

	rec_mon_prepare(mon, false);
	memset(streams, 0, sizeof(streams));
	streams[0].name = "stderr";
	streams[0].fd = mon->stderr_p[PREAD];
	streams[1].name = "stdout";
	streams[1].fd = mon->stdout_p[PREAD];

	rec_log_streams(mon->log, 2, streams, mon->handler, mon->data,
			start_time, stop_time);

	rec_mon_cleanup(mon);
}

/* Run command specified by @cmd and @argv and store its result in @res.
 * @scope defines the scope of data to be recorded (see REC_* definitions).
 * When specified as non-null, @handler is called for each line of data
 * recorded. @data is a data pointer to be passed to @handler. */
void rec_record(struct rec_result *res, char *cmd, char *argv[], int scope,
		linehandler_t handler, void *data)
{
	struct rec_mon mon;

	memset(res, 0, sizeof(*res));

	rec_mon_init(&mon, scope, handler, data);

	gettimeofday(&res->start_time, NULL);

	/* Prevent duplicate output after fork(). */
	fflush(stdout);
	fflush(stderr);

	mon.pid = fork();
	if (mon.pid == -1)
		err(1, "Could not fork");

	if (mon.pid == 0)
		rec_child(&mon, cmd, argv);
	else
		rec_log(&mon, &res->start_time, &res->stop_time);

	timersub(&res->stop_time, &res->start_time, &res->duration);

	/* Synchronize with child and get resource usage.  */
	if (wait4(mon.pid, &res->status, 0, &res->rusage) == -1)
		err(1, "Could not wait on child process");
	res->status_valid = true;
	if (scope & REC_RUSAGE)
		res->rusage_valid = true;

	if (mon.scope & (REC_STDOUT | REC_STDERR)) {
		res->output = mon.log;
		res->output_size = ftell(mon.log);
		rewind(res->output);
		res->output_valid = true;
	} else
		fclose(mon.log);
}

#define RADD(a,b,x)	((a)->x += (b)->x)
#define RMAX(a,b,x)	((a)->x = (a)->x > (b)->x ? (a)->x : (b)->x)

/* a = a + b */
static void rusage_add(struct rusage *a, struct rusage *b)
{
	timeradd(&a->ru_utime, &b->ru_utime, &a->ru_utime);
	timeradd(&a->ru_stime, &b->ru_stime, &a->ru_stime);
	RMAX(a, b, ru_maxrss);
	RADD(a, b, ru_ixrss);
	RADD(a, b, ru_idrss);
	RADD(a, b, ru_isrss);
	RADD(a, b, ru_minflt);
	RADD(a, b, ru_majflt);
	RADD(a, b, ru_nswap);
	RADD(a, b, ru_inblock);
	RADD(a, b, ru_oublock);
	RADD(a, b, ru_msgsnd);
	RADD(a, b, ru_msgrcv);
	RADD(a, b, ru_nsignals);
	RADD(a, b, ru_nvcsw);
	RADD(a, b, ru_nivcsw);
}

#define RSUB(a,b,x)	((a)->x -= (b)->x)

/* a = a - b */
static void rusage_sub(struct rusage *a, struct rusage *b)
{
	timersub(&a->ru_utime, &b->ru_utime, &a->ru_utime);
	timersub(&a->ru_stime, &b->ru_stime, &a->ru_stime);
	/* Cannot subtract maxrss */
	RSUB(a, b, ru_ixrss);
	RSUB(a, b, ru_idrss);
	RSUB(a, b, ru_isrss);
	RSUB(a, b, ru_minflt);
	RSUB(a, b, ru_majflt);
	RSUB(a, b, ru_nswap);
	RSUB(a, b, ru_inblock);
	RSUB(a, b, ru_oublock);
	RSUB(a, b, ru_msgsnd);
	RSUB(a, b, ru_msgrcv);
	RSUB(a, b, ru_nsignals);
	RSUB(a, b, ru_nvcsw);
	RSUB(a, b, ru_nivcsw);
}

/* Print resource usage in @r indented by @indent spaces. */
static void rec_print_rusage(FILE *fd, struct rusage *r, int indent)
{
	PRTIME_MS(fd, "utime_ms: ", &r->ru_utime, indent);
	PRTIME_MS(fd, "stime_ms: ", &r->ru_stime, indent);
	fprintf(fd, "%*smaxrss_kb: %ld\n", indent, "", r->ru_maxrss);
	fprintf(fd, "%*sminflt: %ld\n", indent, "", r->ru_minflt);
	fprintf(fd, "%*smajflt: %ld\n", indent, "", r->ru_majflt);
	fprintf(fd, "%*sinblock: %ld\n", indent, "", r->ru_inblock);
	fprintf(fd, "%*soutblock: %ld\n", indent, "", r->ru_oublock);
	fprintf(fd, "%*snvcsw: %ld\n", indent, "", r->ru_nvcsw);
	fprintf(fd, "%*snivcsw: %ld\n", indent, "", r->ru_nivcsw);
}

/* Print results recorded in @res indented by @indent spaces. */
void rec_print(FILE *fd, struct rec_result *res, int indent)
{
	char *line;
	size_t n;

	if (res->status_valid) {
		if (WIFEXITED(res->status)) {
			fprintf(fd, "%*sexitcode: %d\n", indent, "",
				WEXITSTATUS(res->status));
		}
		if (WIFSIGNALED(res->status)) {
			fprintf(fd, "%*ssignal: %d\n", indent, "",
				WTERMSIG(res->status));
		}
	}
	PRTIME(fd, "starttime: ", &res->start_time, indent);
	PRTIME(fd, "stoptime:  ", &res->stop_time, indent);
	PRTIME_MS(fd, "duration_ms: ", &res->duration, indent);

	if (res->rusage_valid) {
		fprintf(fd, "%*srusage:\n", indent, "");
		rec_print_rusage(fd, &res->rusage, indent + 2);
	}

	if (!res->output_valid)
		return;
	if (res->output_size == 0) {
		fprintf(fd, "%*soutput: \"\"\n", indent, "");
	} else {
		fprintf(fd, "%*soutput: |\n", indent, "");
		rewind(res->output);
		if (res->output) {
			line = NULL;
			while (getline(&line, &n, res->output) != -1) {
				fprintf(fd, "%*s%s", indent + 2, "", line);
				free(line);
				line = NULL;
			}
			free(line);
		}
	}
}

void rec_start(struct rec_result *res, int scope, linehandler_t handler,
	       void *data)
{
	struct rusage usage;
	struct rec_mon *mon;

	memset(res, 0, sizeof(*res));
	res->state = malloc(sizeof(struct rec_mon));
	if (!res->state)
		err(1, "Could not allocate memory");
	mon = res->state;

	rec_mon_init(mon, scope, handler, data);

	gettimeofday(&res->start_time, NULL);

	/* Prevent duplicate output after fork(). */
	fflush(stdout);
	fflush(stderr);

	mon->pid = fork();
	if (mon->pid == -1)
		err(1, "Could not fork");

	if (mon->pid == 0) {
		rec_log(mon, &res->start_time, NULL);
		exit(0);
	}

	rec_mon_prepare(mon, true);
	rec_redirect(scope, mon->stdout_p[PWRITE], mon->stderr_p[PWRITE]);
	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);

	/* Get summary of resource usage so far. */
	getrusage(RUSAGE_SELF, &res->rusage);
	getrusage(RUSAGE_CHILDREN, &usage);
	rusage_add(&res->rusage, &usage);
}

void rec_stop(struct rec_result *res)
{
	struct rusage usage, cusage;
	struct rec_mon *mon = res->state;

	if (mon->scope & REC_RUSAGE) {
		/* Get difference of rusage compared to start. */
		getrusage(RUSAGE_SELF, &usage);
		getrusage(RUSAGE_CHILDREN, &cusage);
		rusage_add(&usage, &cusage);
		rusage_sub(&usage, &res->rusage);
		res->rusage = usage;
		res->rusage_valid = true;
	}

	rec_redirect(mon->scope, mon->orig_stdout, mon->orig_stderr);
	rec_mon_cleanup(mon);
	waitpid(mon->pid, NULL, 0);

	gettimeofday(&res->stop_time, NULL);
	timersub(&res->stop_time, &res->start_time, &res->duration);

	if (mon->scope & (REC_STDOUT | REC_STDERR)) {
		res->output = mon->log;
		res->output_size = ftell(mon->log);
		rewind(res->output);
		res->output_valid = true;
	} else
		fclose(mon->log);
	free(res->state);
}

void rec_close(struct rec_result *res)
{
	if (res->output_valid)
		fclose(res->output);
}

void rec_free_streams(int streamc, struct rec_stream *streams)
{
	int i;

	for (i = 0; i < streamc; i++) {
		free(streams[i].name);
		if (streams[i].fd != -1)
			close(streams[i].fd);
	}
	free(streams);
}
