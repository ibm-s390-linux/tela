/* SPDX-License-Identifier: MIT */
/*
 * Functions to interact with consoles of remote z/VM guest systems.
 *
 * Copyright IBM Corp. 2023
 */

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <regex.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "console_zvm.h"
#include "misc.h"
#include "record.h"

#define S3270_PATH	"/usr/bin/s3270"
#define WAIT_TIMEOUT	20

/* Exit codes (0 and 1 defined in misc.h). */

/* Connection/login process failed or host dropped connection. */
#define EXIT_CONNERR	2

/* A timeout occurred during a #tela expect/#tela idle command. */
#define EXIT_TIMEOUT	3

/* Save only the first reported exit code - there may be more errors reported
 * during shutdown that should be ignored. */
#define SET_EXIT_CODE(d, c)	do { \
	__typeof__(d) _d = (d); \
	__typeof__(c) _c = (c); \
		debug("setting exit code %d", _c); \
		if (_d->exit_code_set) { \
			_d->exit_code_set = true; \
			_d->exit_code = _c; \
		} \
	} while (0)

/* File descriptor to send asynchronous events like timeout to the event
 * loop. */
static int signal_fd;

enum cmd_type {
	/* Send a command to the s3270 process. */
	cmd_type_s3270,
	/* Wait for a console status matching the specified pattern. */
	cmd_type_wait_status,
	/* Wait for a connection status matching the specified pattern. */
	cmd_type_wait_cstate,
	/* Wait for an output line matching the specified pattern. */
	cmd_type_wait_output,
	/* Change wait timeout. */
	cmd_type_set_timeout,
	/* Wait until no more console output arrives within <n> seconds. */
	cmd_type_wait_idle,
	/* Change state of console. */
	cmd_type_set_state,
	/* Print an info message. */
	cmd_type_info,
	/* Print a warning message. */
	cmd_type_warn,
};

enum cons_state {
	/* Not logged in. */
	state_offline = 0,
	/* Telnet connection established. */
	state_connected,
	/* Logged in. */
	state_online,
	/* Logged in + completed set up needed for interaction. */
	state_setup_done,
	/* In the process of logging out. */
	state_logging_off,
};

struct cons_cmd;
struct cons_zvm_data;

typedef void (*cons_cmd_cb_t)(struct cons_zvm_data *, struct cons_cmd *,
			      bool, void *);

struct cons_cmd {
	/* Linked-list pointer. */
	struct cons_cmd *next;

	/* Time when the command was first started. */
	struct timeval start;

	/* Command type. */
	enum cmd_type type;

	/* Type specific input parameters. */
	char *str;
	regex_t pattern;
	int idle_len;
	int idle_count;
	bool user_cmd;

	/* Optional callback that is invoked when command completes. */
	cons_cmd_cb_t cb;
};

#define MSG_LEN	256

static const char *cmd_to_str(struct cons_cmd *cmd)
{
	static char msg[MSG_LEN] = { 0 };

	switch (cmd->type) {
	case cmd_type_s3270:
		snprintf(msg, MSG_LEN, "s3270: %s", cmd->str);
		break;
	case cmd_type_wait_status:
		snprintf(msg, MSG_LEN, "wait_status: %s", cmd->str);
		break;
	case cmd_type_wait_cstate:
		snprintf(msg, MSG_LEN, "wait_cstate: %s", cmd->str);
		break;
	case cmd_type_wait_output:
		snprintf(msg, MSG_LEN, "wait_output: %s", cmd->str);
		break;
	case cmd_type_set_timeout:
		snprintf(msg, MSG_LEN, "set_timeout: %s", cmd->str);
		break;
	case cmd_type_wait_idle:
		snprintf(msg, MSG_LEN, "wait_idle: %s", cmd->str);
		break;
	case cmd_type_set_state:
		snprintf(msg, MSG_LEN, "set_state: %s", cmd->str);
		break;
	case cmd_type_info:
		snprintf(msg, MSG_LEN, "info: %s", cmd->str);
		break;
	case cmd_type_warn:
		snprintf(msg, MSG_LEN, "warn: %s", cmd->str);
		break;
	}
	return msg;
}

static enum cons_state str_to_state(const char *str)
{
	if (strcmp(str, "offline") == 0)
		return state_offline;
	if (strcmp(str, "connected") == 0)
		return state_connected;
	if (strcmp(str, "online") == 0)
		return state_online;
	if (strcmp(str, "setup_done") == 0)
		return state_setup_done;
	if (strcmp(str, "logging_off") == 0)
		return state_logging_off;

	errx(EXIT_RUNTIME, "Internal error: unrecognized state: %s", str);
}

static const char *state_to_str(enum cons_state state)
{
	switch (state) {
	case state_offline:
		return "offline";
	case state_connected:
		return "connected";
	case state_online:
		return "online";
	case state_setup_done:
		return "setup_done";
	case state_logging_off:
		return "logging_off";
	}

	return NULL;
}

/* Add one command with the specified parameters to the queue of commands in
 * @cmds. */
static struct cons_cmd *queue_cons_cmd(struct cons_cmd **cmds,
				       enum cmd_type type, cons_cmd_cb_t cb,
				       const char *fmt, ...)
{
	struct cons_cmd *cmd, *prev;
	size_t errlen;
	char *errmsg;
	int rc;

	get_varargs(fmt, str);

	/* Allocate new command. */
	cmd = misc_malloc(sizeof(*cmd));
	gettimeofday(&cmd->start, NULL);
	cmd->type = type;
	cmd->str = str;
	cmd->cb = cb;

	switch (type) {
	case cmd_type_s3270:
		break;
	case cmd_type_wait_status:
	case cmd_type_wait_cstate:
	case cmd_type_wait_output:
		rc = regcomp(&cmd->pattern, str, REG_EXTENDED | REG_NOSUB);
		if (rc) {
			errlen = regerror(rc, &cmd->pattern, NULL, 0);
			errmsg = misc_malloc(errlen);
			regerror(rc, &cmd->pattern, errmsg, errlen);
			warnx("Skipping command due to invalid regexp '%s': %s",
			      str, errmsg);
			free(errmsg);
			free(str);
			free(cmd);

			return NULL;
		}
		break;
	case cmd_type_wait_idle:
		/* Number of output lines since last check. */
		cmd->idle_count = 0;
		/* Fill in user-provided value if available. */
		if (sscanf(str, "%d", &cmd->idle_len) != 1) {
			/* Default length of required idle period in seconds. */
			cmd->idle_len = 1;
		}
		break;
	case cmd_type_set_timeout:
	case cmd_type_set_state:
	case cmd_type_info:
	case cmd_type_warn:
		break;
	}

	/* Tail-queue new command. */
	for (prev = *cmds; prev && prev->next; prev = prev->next)
		;

	if (prev)
		prev->next = cmd;
	else
		*cmds = cmd;

	return cmd;
}

/* Shortcuts. */
#define queue_s3270_cmd(d, fmt, ...) \
	queue_cons_cmd(&((d)->cmds), cmd_type_s3270, NULL, fmt, ##__VA_ARGS__)
#define queue_wait_status(d, fmt, ...) \
	queue_cons_cmd(&((d)->cmds), cmd_type_wait_status, NULL, fmt, \
		       ##__VA_ARGS__)
#define queue_wait_cstate(d, fmt, ...) \
	queue_cons_cmd(&((d)->cmds), cmd_type_wait_cstate, NULL, fmt, \
		       ##__VA_ARGS__)
#define queue_wait_output(d, fmt, ...) \
	queue_cons_cmd(&((d)->cmds), cmd_type_wait_output, NULL, fmt, \
		       ##__VA_ARGS__)

static struct cons_cmd *unqueue_cons_cmd(struct cons_cmd **cmds)
{
	struct cons_cmd *cmd = NULL;

	if (*cmds) {
		cmd = *cmds;
		*cmds = cmd->next;
	}

	return cmd;
}

static void free_cons_cmd(struct cons_cmd *cmd)
{
	if (!cmd)
		return;

	switch (cmd->type) {
	case cmd_type_wait_status:
	case cmd_type_wait_cstate:
	case cmd_type_wait_output:
		regfree(&cmd->pattern);
		break;
	default:
		break;
	}

	free(cmd->str);
	free(cmd);
}

struct cons_zvm_data {
	/* The write portion of the pipe that ends in the s3270 stdin pipe. */
	int s_stdin_fd;

	/* The list of commands to perform.*/
	struct cons_cmd *cmds;

	/* The current command that is being processed. */
	struct cons_cmd *curr_cmd;

	/* The first line of s3270 command result output. */
	char *out_line1;

	/* The previous trace lines in a multi-line trace output. */
	char *trace;

	/* The status line in the 3270 output window. */
	char *cons_status;

	/* The s3270 connection state. */
	char *cstate;

	/* State of console interaction. */
	enum cons_state state;

	/* While true, all output is saved to @output_lines. */
	bool save_output;

	/* Concatenation of output lines that were written to the console
	 * while save_output was active. */
	char *output_lines;

	/* Exit code - 0 in case console interaction worked as planned,
	 * 1 in case of connection problems/disconnect, 2 in case of timeout. */
	int exit_code;

	/* Flag to prevent overwriting of initial exit code. */
	bool exit_code_set;

	/* Number of seconds after which to stop waiting. */
	int timeout;
};

static bool match_pattern(regex_t *re, const char *string)
{
	bool rc = (regexec(re, string, 0, NULL, 0) == 0);

	debug2("string=%s rc=%d", string, rc);

	return rc;
}

static void info(const char *fmt, ...)
{
	get_varargs(fmt, str);

	misc_chomp(str);
	printf("%s: %s\n", program_invocation_short_name, str);
	free(str);
}

static void flush_cmds(struct cons_zvm_data *d)
{
	struct cons_cmd *cmd;

	while ((cmd = unqueue_cons_cmd(&d->cmds)))
		free_cons_cmd(cmd);

	/* Also turn of output save since it is associated with a queued
	 * command. */
	if (d->save_output) {
		d->save_output = false;
		free(d->output_lines);
		d->output_lines = NULL;
	}
}

/* Consume saved output lines. Return %true if wait condition was completed
 * with saved output lines, %false otherwise. */
static bool handle_saved_output(struct cons_zvm_data *d, struct cons_cmd *cmd,
				char **line)
{
	bool rc = false;
	char *s;

	if (!d->save_output)
		return false;

	if (!d->output_lines)
		goto out;

	for (s = strtok(d->output_lines, "\n"); s && !rc;
	     s = strtok(NULL, "\n")) {
		rc = match_pattern(&cmd->pattern, s);
		if (rc)
			*line = misc_strdup(s);
	}

	if (rc)
		debug("saved output '%s' matched pattern '%s'", *line,
		      cmd->str);

	free(d->output_lines);
	d->output_lines = NULL;

out:
	d->save_output = false;

	return rc;
}

static void complete_cmd(struct cons_zvm_data *d, bool rc, void *data)
{
	struct cons_cmd *cmd = d->curr_cmd;

	d->curr_cmd = NULL;

	if (cmd->cb)
		cmd->cb(d, cmd, rc, data);

	if (rc) {
		debug("Completed command %s", cmd_to_str(cmd));
	} else {
		debug("Aborted command %s", cmd_to_str(cmd));
	}

	/* Disarm alarm in case of wait commands. */
	if (cmd->type == cmd_type_wait_status ||
	    cmd->type == cmd_type_wait_cstate ||
	    cmd->type == cmd_type_wait_output ||
	    cmd->type == cmd_type_wait_idle) {
		alarm(0);
	}

	free_cons_cmd(cmd);
}

/* Perform command, return %true if command was completed, %false if completion
 * will be indicated asynchronously. */
static bool perform_cmd(struct cons_zvm_data *d, struct cons_cmd *cmd)
{
	char *line = NULL;
	void *data = NULL;
	bool rc = true;

	switch (cmd->type) {
	case cmd_type_s3270:
		debug("send s3270 cmd '%s'", cmd->str);

		/* Send s3270 command to stdin of s3270 process. */
		if (write(d->s_stdin_fd, cmd->str, strlen(cmd->str)) == -1) {
			warn("Could not write to s3270 stdin");
			break;
		}

		/* Ensure newline. */
		if (!misc_ends_with(cmd->str, "\n") &&
		    write(d->s_stdin_fd, "\n", 1) == -1) {
			warn("Could not write to s3270 stdin");
			break;
		}

		/* Save output lines if next queued command is waiting for
		 * specific output. */
		if (d->cmds && d->cmds->type == cmd_type_wait_output)
			d->save_output = true;

		rc = false;
		break;
	case cmd_type_wait_status:
		debug("wait for status '%s'", cmd->str);
		rc = (d->cons_status &&	match_pattern(&cmd->pattern,
						      d->cons_status));
		if (!rc)
			alarm(d->timeout);
		break;
	case cmd_type_wait_cstate:
		debug("wait for cstate '%s'", cmd->str);
		rc = (d->cstate && match_pattern(&cmd->pattern, d->cstate));
		if (!rc)
			alarm(d->timeout);
		break;
	case cmd_type_wait_output:
		debug("wait for output '%s'", cmd->str);
		rc = handle_saved_output(d, cmd, &line);
		if (rc)
			data = line;
		else
			alarm(d->timeout);
		break;
	case cmd_type_wait_idle:
		debug("wait for %d seconds for idle output",
		      cmd->idle_len);

		if (d->timeout > 0 && d->timeout < cmd->idle_len) {
			warnx("Idle period longer than wait timeout - "
			      "skipping");
			break;
		}
		alarm(cmd->idle_len);
		rc = false;
		break;
	case cmd_type_set_timeout:
		d->timeout = atoi(cmd->str);
		if (d->timeout == 0)
			info("Disabling timeout");
		else
			info("Setting wait timeout to %d seconds", d->timeout);
		break;
	case cmd_type_set_state:
		debug("changing state %s -> %s", state_to_str(d->state),
		      cmd->str);
		d->state = str_to_state(cmd->str);
		break;
	case cmd_type_info:
		if (d->state != state_logging_off)
			info("%s", cmd->str);
		break;
	case cmd_type_warn:
		if (d->state != state_logging_off)
			warnx("%s", cmd->str);
		break;
	}

	if (rc) {
		debug("command completed synchronously");
		complete_cmd(d, true, data);
	} else {
		debug("waiting for asynchronous command completion");
	}
	free(line);

	return rc;
}

/* Start next queued command if console is currently not busy. */
static void kick_cons_cmd(struct cons_zvm_data *d)
{
	if (d->curr_cmd || !d->cmds)
		return;

	while ((d->curr_cmd = unqueue_cons_cmd(&d->cmds))) {
		if (!perform_cmd(d, d->curr_cmd)) {
			/* Wait for asynchronous command completion. */
			break;
		}
	}
}

static void queue_quit(struct cons_zvm_data *d, bool flush, bool disc,
		       int exit_code, const char *fmt, ...)
{
	get_varargs(fmt, str);

	debug("flush=%d disc=%d reason='%s'", flush, disc, str);

	SET_EXIT_CODE(d, exit_code);
	if (flush) {
		if (d->curr_cmd)
			complete_cmd(d, false, NULL);

		flush_cmds(d);
	}

	if (str && *str) {
		queue_cons_cmd(&d->cmds, exit_code == EXIT_OK ? cmd_type_info
							      : cmd_type_warn,
			       NULL, str);
	}
	queue_cons_cmd(&d->cmds, cmd_type_set_state, NULL, "logging_off");
	if (disc) {
		queue_s3270_cmd(d, "string \"#cp disc\"");
		queue_s3270_cmd(d, "enter");
		queue_wait_output(d, "DISCONNECT AT");
	}
	queue_s3270_cmd(d, "quit");

	free(str);
}

static void handle_event_timeout(struct cons_zvm_data *d)
{
	struct cons_cmd *cmd = d->curr_cmd;
	bool disc = true;
	char *msg;

	debug("Timeout occurred");

	if (d->state == state_offline) {
		msg = misc_strdup("Connection failed - timeout during logon");
		disc = false;
	} else if (d->state == state_online) {
		msg = misc_strdup("Connection failed - timeout during setup");
	} else if (cmd && cmd->type == cmd_type_wait_output) {
		msg = misc_asprintf("Timed out waiting for output matching "
				    "pattern: %s", cmd->str);
	} else {
		msg = misc_strdup("Timed out waiting for response - closing "
				  "connection");
	}

	queue_quit(d, true, disc, EXIT_CONNERR, "%s", msg);
	free(msg);
}

static void handle_event_alarm(struct cons_zvm_data *d)
{
	struct cons_cmd *cmd = d->curr_cmd;
	struct timeval now;
	time_t s;

	if (!cmd) {
		debug("Timeout without pending command");
		return;
	}

	switch (cmd->type) {
	case cmd_type_wait_idle:
		if (cmd->idle_count == 0) {
			/* No output during idle period. */
			complete_cmd(d, true, NULL);
			return;
		}

		gettimeofday(&now, NULL);
		s = now.tv_sec - cmd->start.tv_sec;

		if (s + cmd->idle_len > d->timeout) {
			/* Next wait would exceed timeout. */
			break;
		}

		/* Re-arm alarm timer. */
		cmd->idle_count = 0;
		alarm(cmd->idle_len);
		return;
	default:
		break;
	}

	if (cmd->user_cmd) {
		info("User command timed out");
		complete_cmd(d, false, "Command timed out");

		SET_EXIT_CODE(d, EXIT_TIMEOUT);
	} else {
		handle_event_timeout(d);
	}
}

static void handle_event_output(struct cons_zvm_data *d, char *line)
{
	struct cons_cmd *cmd = d->curr_cmd;
	char *saved_output;

	/* Check if output is being waited for. */
	if (cmd) {
		switch (cmd->type) {
		case cmd_type_wait_output:
			if (match_pattern(&cmd->pattern, line)) {
				debug("output '%s' matched pattern '%s'", line,
				      cmd->str);
				complete_cmd(d, true, line);
			}
			break;
		case cmd_type_wait_idle:
			/* Note output line during idle period. */
			cmd->idle_count++;
			break;
		default:
			break;
		}
	}

	/* Print console output. */
	if (d->state == state_setup_done)
		printf("%s\n", line);
	else
		debug("suppressed console output '%s'", line);

	/* If requested, save output. */
	if (d->save_output) {
		saved_output = d->output_lines;
		d->output_lines =
			misc_asprintf("%s\n%s",
				      saved_output ? saved_output : "", line);
		free(saved_output);
	}
}

static void handle_event_status(struct cons_zvm_data *d, char *status)
{
	struct cons_cmd *cmd = d->curr_cmd;

	debug("console status changed to '%s'", status);

	free(d->cons_status);
	d->cons_status = misc_strdup(status);

	/* Check if status is being waited for. */
	if (cmd && cmd->type == cmd_type_wait_status &&
	    match_pattern(&cmd->pattern, status)) {
		debug("status '%s' matched pattern '%s'", status, cmd->str);
		complete_cmd(d, true, status);
	}
}

#define TELA_CMD_PREFIX		"#tela"

/* #tela timeout <timeout>
 *
 * Set timeout in seconds for tela commands that wait (expect, idle). After
 * timeout expired, the wait command will continue even if the wait condition
 * was not met. Default is 20s. Set to 0 to disable timeout handling. */
#define TELA_CMD_TIMEOUT	TELA_CMD_PREFIX " timeout"

/* #tela expect <expr>
 *
 * Wait for output matching the specified expression to appear. The expression
 * must be provided in POSIX Extended Regular Expression syntax. */
#define TELA_CMD_WAIT_OUTPUT	TELA_CMD_PREFIX " expect"

/* #tela idle [<value>]
 *
 * Wait until no output occurred for the specified number of seconds. If no
 * value was specified, the default of 1s is used. */
#define TELA_CMD_WAIT_IDLE	TELA_CMD_PREFIX " idle"

static void handle_tela_cmd(struct cons_zvm_data *d, char *line)
{
	struct cons_cmd *cmd = NULL;
	char *p;

	if (misc_starts_with(line, TELA_CMD_TIMEOUT)) {
		p = line + strlen(TELA_CMD_TIMEOUT);
		misc_skip_space(p);
		cmd = queue_cons_cmd(&d->cmds, cmd_type_set_timeout, NULL, p);
	} else if (misc_starts_with(line, TELA_CMD_WAIT_OUTPUT)) {
		p = line + strlen(TELA_CMD_WAIT_OUTPUT);
		misc_skip_space(p);
		cmd = queue_cons_cmd(&d->cmds, cmd_type_wait_output, NULL, p);
	} else if (misc_starts_with(line, TELA_CMD_WAIT_IDLE)) {
		p = line + strlen(TELA_CMD_WAIT_IDLE);
		misc_skip_space(p);
		cmd = queue_cons_cmd(&d->cmds, cmd_type_wait_idle, NULL, p);
	}  else {
		warnx("Unknown tela command: %s", line);
	}

	if (cmd)
		cmd->user_cmd = true;
}

static void handle_event_input(struct cons_zvm_data *d, char *line)
{
	debug("got user input '%s'", line);

	if (misc_starts_with(line, TELA_CMD_PREFIX)) {
		handle_tela_cmd(d, line);
		return;
	}

	/* Send user input as console input to s3270 process. */
	line = misc_escape(line, "\"");
	queue_s3270_cmd(d, "string \"%s\"", line);
	queue_s3270_cmd(d, "enter");
	free(line);
}

static void handle_event_input_hangup(struct cons_zvm_data *d)
{
	debug("stdin was closed");

	queue_quit(d, false, true, EXIT_OK, "EOF on standard input - closing "
		   "connection");
}

static void handle_event_s3270_response(struct cons_zvm_data *d, char *line1,
					char *line2)
{
	debug("s3270 response line1='%s' line2='%s'", line1, line2);

	/* Second line is command response. */
	if (strcmp(line2, "ok") != 0) {
		warnx("Internal error: s3270 rejected command '%s'",
		      d->curr_cmd->str);
		queue_quit(d, true, false, EXIT_RUNTIME, "Internal error: "
			   "s3270 rejected command '%s'", d->curr_cmd->str);
		return;
	}

	if (line1[0] == 'L' && d->state != state_logging_off) {
		queue_quit(d, true, false, EXIT_CONNERR, "Host closed "
			   "connection");
		return;
	}

	complete_cmd(d, true, line1);
}

static void handle_trace_write_line(struct cons_zvm_data *d, char *line)
{
	char *curr, *next, *start, *end;
	int row, col;
	bool prot;

	/*
	 * Expect:
	 *
	 *  SetBufferAddress(<row>,1)
	 *    SetAttribute highlighting(default)
	 *    SetAttribute foreground(default)
	 *    '<console output>'
	 *    SetAttribute all(0)
	 *
	 * or
	 *
	 *  SetBufferAddress(43,60)
	 *    StartFieldExtended foreground(default) highlighting(default)
	 *                       3270(protected)
	 *    '<console status>'
	 *    StartField(43,79)(protected)
	 */
	for (curr = strstr(line, "SetBufferAddress("); curr; curr = next) {
		next = strstr(curr + 1, "SetBufferAddress(");
		if (next)
			next[-1] = 0;

		if (sscanf(curr, "SetBufferAddress(%d,%d)", &row, &col) != 2) {
			/* Unknown format. */
			continue;
		}

		start = strchr(curr, '\'');
		end = strrchr(curr, '\'');
		if (!start || !end || start == end) {
			/* Missing or invalid text output. */
			continue;
		}
		start++;
		prot = misc_ends_with(start, "(protected)");
		*end = 0;

		if (prot) {
			/* Text in protected field = console status. */
			handle_event_status(d, start);
		} else {
			/* Text in non-protected field = console output. */
			handle_event_output(d, start);
		}
	}
}

static void handle_disconnect(struct cons_zvm_data *d)
{
	if (d->state < state_connected || d->state == state_logging_off)
		return;

	queue_quit(d, true, false, EXIT_CONNERR, "Host closed connection");
}

static void handle_event_cstate(struct cons_zvm_data *d, char *cstate)
{
	struct cons_cmd *cmd = d->curr_cmd;

	debug("connection status changed to '%s'", cstate);

	free(d->cstate);
	d->cstate = misc_strdup(cstate);

	/* Check if status is being waited for. */
	if (cmd && cmd->type == cmd_type_wait_cstate &&
	    match_pattern(&cmd->pattern, cstate)) {
		debug("cstate '%s' matched pattern '%s'", cstate, cmd->str);
		complete_cmd(d, true, cstate);
	} else if (strcmp(cstate, "not-connected") == 0)
		handle_disconnect(d);
}

static void handle_trace_cstate_line(struct cons_zvm_data *d, char *line)
{
	char *cstate, *end;

	/* cstate [telnet-pending] -> [connected-3270] */
	cstate = strrchr(line, '[');
	end = strrchr(line, ']');
	if (!cstate || !end)
		return;
	cstate++;
	*end = 0;

	handle_event_cstate(d, cstate);
}

static void handle_trace_line(struct cons_zvm_data *d, char *line)
{
	if (misc_starts_with(line, "< Write"))
		handle_trace_write_line(d, line);
	else if (strstr(line, "cstate ["))
		handle_trace_cstate_line(d, line);
	else if (strstr(line, "Keyboard lock") &&
		 strstr(line, "+NOT_CONNECTED"))
		handle_disconnect(d);
}

static void handle_trace_output(struct cons_zvm_data *d, char *line)
{
	if (misc_starts_with(line, "... ")) {
		/* Continuation of previous line. */
		line += 4;
		line = misc_asprintf("%s%s", d->trace ? d->trace : "", line);
		free(d->trace);
		d->trace = NULL;
	} else
		line = misc_strdup(line);

	if (misc_ends_with(line, " ...")) {
		/* Line will be continued. */
		line[strlen(line) - 4] = 0;
		d->trace = line;
		return;
	}

	/* Consume line. */
	handle_trace_line(d, line);

	free(line);
}

static void logon_check_cb(struct cons_zvm_data *d, struct cons_cmd *cmd,
			   bool rc, void *data)
{
	char *line = data;

	if (!rc || !line || !misc_starts_with(line, "HCP")) {
		d->state = state_online;
		return;
	}

	/* CP Error message. */
	queue_quit(d, true, false, EXIT_CONNERR, "%s", line);
}

static void alrm_handler(int signum)
{
	char msg[] = "a\n";

	debug("got alarm signal %d", signum);
	write(signal_fd, msg, strlen(msg));
}

static void quit_handler(int signum)
{
	char msg[] = "q\n";

	debug("got quit signal %d", signum);
	write(signal_fd, msg, strlen(msg));
}

static void cons_zvm_handler(void *data, char *line, struct rec_stream *stream)
{
	struct cons_zvm_data *d = data;

	if (line)
		misc_chomp(line);

	if (strcmp(stream->name, "stdin") == 0) {
		/* User input via stdin. */
		if (line)
			handle_event_input(d, line);
		else
			handle_event_input_hangup(d);
	} else if (strcmp(stream->name, "trace") == 0) {
		/* s3270 trace output. */
		debug2("trace output: %s", line);
		handle_trace_output(d, line);
	} else if (strcmp(stream->name, "s_stdout") == 0) {
		/* s3270 program output. */
		debug2("s3270 output: %s", line);

		if (!d->out_line1) {
			d->out_line1 = misc_strdup(line);
		} else {
			handle_event_s3270_response(d, d->out_line1, line);
			free(d->out_line1);
			d->out_line1 = NULL;
		}
	} else if (strcmp(stream->name, "signal") == 0) {
		if (strcmp(line, "a") == 0) {
			debug("alarm");
			handle_event_alarm(d);
		} else if (strcmp(line, "q") == 0) {
			debug("killed by signal");
			queue_quit(d, true, d->state != state_offline,
				   EXIT_RUNTIME, "Killed by signal");
		}
	} else {
		/* Pass stderr of s3270 process through to own stderr. */
		fprintf(stderr, "s3270: %s\n", line);
	}

	kick_cons_cmd(d);
}

/* Connect to the z/VM Hypervisor console. */
int cons_zvm_run(char *host, char *user, char *pass, bool keep_open)
{
	int s_stdin[2], s_stdout[2], s_stderr[2], signal_fds[2], trace_fd;
	char *tmpdir, *fifo, *argv[8];
	struct rec_stream streams[5];
	struct cons_zvm_data data;
	struct timeval start;
	pid_t s_pid;

	/* Check for required tool. */
	if (!misc_exists(S3270_PATH))
		errx(EXIT_RUNTIME, "Missing required tool %s", S3270_PATH);

	/* Create a temporary FIFO used as tracefile. */
	tmpdir = misc_mktempdir(NULL);
	fifo = misc_asprintf("%s/fifo", tmpdir);
	if (mkfifo(fifo, 0600) != 0)
		err(EXIT_RUNTIME, "Could not create FIFO %s", fifo);

	/* Spawn an s3270 instance. */
	if (pipe(s_stdin) == -1 || pipe(s_stdout) == -1 || pipe(s_stderr) == -1)
		err(EXIT_RUNTIME, "Could not create pipes");

	s_pid = fork();
	if (s_pid == -1)
		err(EXIT_RUNTIME, "Could not fork");
	if (s_pid == 0) {
		/* Child process.  */

		/* Prevent propagation of signals sent to parent. */
		setsid();

		/* Prepare streams. */
		close(s_stdin[PWRITE]);
		close(s_stdout[PREAD]);
		close(s_stderr[PREAD]);
		if (dup2(s_stdin[PREAD], STDIN_FILENO) == -1)
			err(EXIT_RUNTIME, "Could not redirect child stdin");
		if (dup2(s_stdout[PWRITE], STDOUT_FILENO) == -1)
			err(EXIT_RUNTIME, "Could not redirect child stdout");
		if (dup2(s_stderr[PWRITE], STDERR_FILENO) == -1)
			err(EXIT_RUNTIME, "Could not redirect child stderr");

		/* Start s3270 command. */
		argv[0] = S3270_PATH;
		argv[1] = "-trace";
		argv[2] = "-tracefile";
		argv[3] = fifo;
		argv[4] = "-charset";
		argv[5] = "us-intl";
		argv[6] = host;
		argv[7] = NULL;
		execv(S3270_PATH, argv);

		errx(EXIT_RUNTIME, "Could not start child process");
	}

	/* Parent process. */
	close(s_stdin[PREAD]);
	close(s_stdout[PWRITE]);
	close(s_stderr[PWRITE]);

	trace_fd = open(fifo, O_RDONLY);
	if (trace_fd == -1)
		err(EXIT_RUNTIME, "Could not open pipe");

	/* Open pipe for receiving signals. */
	if (pipe(signal_fds) == -1)
		err(EXIT_RUNTIME, "Could not create pipes");
	signal_fd = signal_fds[PWRITE];

	signal(SIGALRM, alrm_handler);
	signal(SIGQUIT, quit_handler);
	signal(SIGINT, quit_handler);
	signal(SIGTERM, quit_handler);

	/* Process data from all FDs. */
	memset(streams, 0, sizeof(streams));
	streams[0].fd = STDIN_FILENO;
	streams[0].name = "stdin";
	streams[0].nocount = true;
	streams[0].onclose = !keep_open;
	streams[1].fd = trace_fd;
	streams[1].name = "trace";
	streams[2].fd = s_stdout[PREAD];
	streams[2].name = "s_stdout";
	streams[3].fd = s_stderr[PREAD];
	streams[3].name = "s_stderr";
	streams[4].fd = signal_fds[PREAD];
	streams[4].name = "signal";
	streams[4].nocount = true;

	memset(&data, 0, sizeof(data));
	data.s_stdin_fd = s_stdin[PWRITE];
	data.cons_status = misc_strdup("unknown");
	data.timeout = WAIT_TIMEOUT;

	/* Queue initial s3270 commands to perform logon. */
	queue_wait_cstate(&data, "connected-3270");
	queue_s3270_cmd(&data, "clear");

	queue_wait_status(&data, "CP READ");
	queue_s3270_cmd(&data, "string \"logon %s here\"", user);
	queue_s3270_cmd(&data, "enter");

	queue_wait_status(&data, "CP READ");
	queue_s3270_cmd(&data, "string \"%s\"", pass);
	queue_s3270_cmd(&data, "enter");

	queue_cons_cmd(&data.cmds, cmd_type_wait_output, logon_check_cb,
		       "(LOGON AT|RECONNECTED AT|HCP\\S+E)");
	queue_s3270_cmd(&data, "string \"#CP TERM MORE 0 0\"");
	queue_s3270_cmd(&data, "enter");

	queue_wait_output(&data, "TERM MORE");
	queue_s3270_cmd(&data, "string \"#CP TERM HOLD OFF\"");
	queue_s3270_cmd(&data, "enter");

	queue_wait_output(&data, "HOLD OFF");
	queue_s3270_cmd(&data, "string \"#CP SET RUN ON\"");
	queue_s3270_cmd(&data, "enter");

	queue_wait_output(&data, "SET RUN ON");
	queue_cons_cmd(&data.cmds, cmd_type_set_state, NULL, "setup_done");
	queue_cons_cmd(&data.cmds, cmd_type_info, NULL, "Connected to %s at %s",
		       user, host);

	kick_cons_cmd(&data);

	/*
	 * Event loop:
	 *  - s3270 stdout: wait for ack to commands
	 *  - s3270 stderr: write through to own stderr
	 *  - s3270 trace:  parse and handle console output
	 *  - own stdin:    convert to s3270 commands and send to s3270
	 *  - signal_fd:    handle timeouts and termination signals
	 */
	debug("Entering event loop");
	gettimeofday(&start, NULL);
	rec_log_streams(NULL, ARRAY_SIZE(streams), streams, cons_zvm_handler,
			&data, NULL, NULL);
	debug("Leaving event loop");

	if (data.state != state_logging_off) {
		SET_EXIT_CODE(&data, EXIT_RUNTIME);
		warnx("s3270 process terminated unexpectedly");
	}

	signal(SIGALRM, SIG_DFL);
	signal(SIGQUIT, SIG_DFL);
	signal(SIGINT, SIG_DFL);
	signal(SIGTERM, SIG_DFL);

	flush_cmds(&data);
	free_cons_cmd(data.curr_cmd);
	free(data.out_line1);
	free(data.trace);
	free(data.cons_status);
	free(data.output_lines);

	close(trace_fd);
	close(signal_fds[PREAD]);
	close(signal_fds[PWRITE]);
	close(s_stdin[PWRITE]);
	close(s_stdout[PREAD]);
	close(s_stderr[PREAD]);

	free(fifo);
	free(tmpdir);

	return data.exit_code;
}
