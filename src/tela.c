/* SPDX-License-Identifier: MIT */
/*
 * Main tela command line tool.
 *
 * Copyright IBM Corp. 2023
 */

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <fnmatch.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "config.h"
#include "console_zvm.h"
#include "log.h"
#include "misc.h"
#include "pretty.h"
#include "record.h"
#include "resource.h"
#include "yaml.h"

#define CMD_COUNT	"count"
#define CMD_MONITOR	"monitor"
#define CMD_RUN		"run"
#define CMD_FORMAT	"format"
#define CMD_EVAL	"eval"
#define CMD_YAMLGET	"yamlget"
#define CMD_FIXNAME	"fixname"
#define CMD_MATCH	"match"
#define CMD_CONSOLE	"console"
#define CMD_YAMLSCALAR	"yamlscalar"

/* A mapping of characters that need to be escaped for consumption in shell
 * single quotes. */
static struct misc_map shell_escape_single_map[] = {
	{ "'", "'\\''" },
	{ "\n", "'\"\\n\"'" },
	{ NULL, NULL }
};

/* A mapping of characters that need to be escaped for consumption in shell
 * double quotes. */
static struct misc_map shell_escape_double_map[] = {
	{ "\\", "\\\\" },
	{ "\n", "\\n" },
	{ "$", "\\$" },
	{ "\"", "\\\"" },
	{ "`", "\\`" },
	{ NULL, NULL },
};

/* Reversal of shell_escape_map. */
static struct misc_map shell_unescape_double_map[] = {
	{ "\\n", "\n" },
	{ "\\$", "$" },
	{ "\\\"", "\"" },
	{ "\\`", "`" },
	{ "\\\\", "\\" },
	{ NULL, NULL },
};

static void usage(void)
{
	static const char * const cmds[] = {
		CMD_COUNT, CMD_MONITOR, CMD_RUN, CMD_FORMAT, CMD_EVAL,
		CMD_YAMLGET, CMD_FIXNAME, CMD_MATCH, CMD_CONSOLE,
		CMD_YAMLSCALAR, NULL,
	};
	int i;

	fprintf(stdout, "Supported commands:");
	for (i = 0; cmds[i]; i++)
		fprintf(stdout, " %s", cmds[i]);
	fprintf(stdout, "\n");
}

/* Return plan for a single testexec. */
static int count_one(const char *exec)
{
	struct config_t cfg;
	struct stat buf;

	/* Filter out directories. */
	if (stat(exec, &buf) == 0 && S_ISDIR(buf.st_mode))
		return 0;

	config_read(&cfg, "%s.yaml", exec);

	return cfg.plan > 0 ? cfg.plan : 1;
}

/* Print combined number of tests implemented by test executables in argv. */
static int cmd_count(int argc, char *argv[])
{
	int i;
	int count = 0;

	for (i = 0; i < argc; i++)
		count += count_one(argv[i]);

	printf("%d\n", count);

	return 0;
}

/* Collect stdout and stderr output data from the specified FIFOs and generate
 * TAP13-yaml output from it. */
static int cmd_monitor(int argc, char *argv[])
{
	struct rec_stream *streams;
	struct timeval start;
	char *d;
	int i;

	if (argc < 1) {
		fprintf(stderr, "Usage: %s %s <name>:<path> ...\n",
			program_invocation_short_name, CMD_MONITOR);
		exit(EXIT_SYNTAX);
	}

	streams = misc_malloc(sizeof(struct rec_stream) * (argc + 1));
	for (i = 0; i < argc; i++) {
		d = strchr(argv[i], ':');
		if (!d) {
			errx(EXIT_SYNTAX, "Missing colon in argument '%s'",
			     argv[i]);
		}
		*d = 0;
		d++;
		streams[i].name = misc_strdup(argv[i]);
		streams[i].fd = open(d, O_RDONLY);
		if (streams[i].fd == -1)
			err(EXIT_RUNTIME, "Could not open file '%s'", d);
		/* We're only interested in new data. */
		lseek(streams[i].fd, 0, SEEK_END);
	}

	/* Accept control data from standard input. */
	streams[i].fd = STDIN_FILENO;

	/* Ensure that output reaches target even when redirected to file. */
	setlinebuf(stdout);

	gettimeofday(&start, NULL);
	rec_log_streams(stdout, argc + 1, streams, NULL, NULL, &start, NULL);
	rec_free_streams(argc, streams);

	return 0;
}

struct runlog_data {
	FILE *fd;
	struct timeval start;
	char *exec;
};

struct run_data {
	bool check_done;
	bool is_tap13;
	int num;
	int plan;
	bool large_temp;
	char *exec;
	char *exec_dir;
	const char *rexec;
	char *last_stderr;
	char **env;
	struct yaml_node *desc;
	char *matchfile;
	struct runlog_data runlog;
};

/* Parse testexec TAP output. */
static void handle_tap_line(struct run_data *data, char *line,
			    struct rec_stream *stream)
{
	char *s, *name, *reason;
	enum tela_result_t result;
	int num;
	struct yaml_node *node;

	if (strcmp(stream->name, "stdout") != 0) {
		/* A harness must only read TAP output from standard output. */
		twarn(data->exec, 0, "%s", line);
	} else if (strncmp(line, "TAP ", 4) == 0) {
		/* Filter out TAP header. */
	} else if (log_parse_plan(line, &num)) {
		/* Got a test plan. */
		if (data->plan != -1) {
			if (data->plan != num) {
				twarn(data->exec, 0, "Plan in TAP output "
				      "(%d) does not match expected plan "
				      "(%d)\n", num, data->plan);
			}
		} else {
			printf("%s", line);
			data->plan = num;
		}
	} else if (log_parse_line(line, &s, &num, &result, &reason)) {
		/* Got a test result line - convert to canonical form. */
		data->num++;
		if (s) {
			/* Check for invalid characters. */
			name = misc_strdup(s);
			misc_fix_testname(s);
			if (strcmp(name, s) != 0) {
				twarn(data->rexec, 0, "Invalid characters in "
				      "test name '%s': only use 0-9a-zA-Z._-",
				      name);
			}
			free(name);

			/* mark node as handled */
			node = yaml_get_node(data->desc, s);
			if (node)
				node->handled = true;

			name = misc_asprintf("%s:%s", data->rexec, s);
		} else {
			if (num == -1)
				num = data->num;
			name = misc_asprintf("%s:%d", data->rexec, num);
		}

		log_line(stdout, data->num, name, result, reason);

		free(name);
		free(s);
		free(reason);
	} else if (log_parse_bail(line)) {
		/* Terminate test run. */
		reason = strchr(line, '!');
		reason++;
		while (isspace(*reason))
			reason++;

		if (*reason)
			printf("Bail out! %s: %s", data->rexec, reason);
		else
			printf("Bail out! %s\n", data->rexec);
		exit(1);
	} else if (line[0] != ' ' && line[0] != '#') {
		/* TAP13 test produced non-TAP13 output - emit warning. */
		twarn(data->exec, 0, "Output not in TAP13 format: %s", line);
	} else {
		/* Pass anything else through. */
		printf("%s", line);
	}
}

/* Parse testexec non-TAP exec. */
static void handle_nontap_line(struct run_data *data, char *line,
			       struct rec_stream *stream)
{
	/* Note last line printed to stderr for use as skip/todo reason. */
	if (strcmp(stream->name, "stderr") == 0) {
		free(data->last_stderr);
		data->last_stderr = misc_strdup(line);
	}
}

/* Write a line to the runlog. */
static void runlog_puts(struct runlog_data *log, const char *line)
{
	struct timeval tv;
	bool nl;

	if (!log->fd)
		return;

	gettimeofday(&tv, NULL);
	timersub(&tv, &log->start, &tv);

	/* Write line header. */
	nl = misc_ends_with(line, "\n");
	fprintf(log->fd, "[%4lu.%06lu] %s%s%s", (unsigned long) tv.tv_sec,
		(unsigned long) tv.tv_usec, nl ? " " : "(nonl) ",
		line, nl ? "" : "\n");
}

static void runlog_printf(struct runlog_data *log, const char *fmt, ...)
{
	va_list args;
	char *str;
	int rc;

	va_start(args, fmt);
	rc = vasprintf(&str, fmt, args);
	va_end(args);
	if (rc == -1)
		oom();

	runlog_puts(log, str);
	free(str);
}

static void runlog_open(struct runlog_data *runlog, const char *logfile,
			const char *exec)
{
	FILE *fd;
	struct timeval now;

	/* Open for appending to allow multiple tela invocations to share the
	 * same logfile. */
	fd = fopen(logfile, "a");
	if (!fd) {
		err(EXIT_RUNTIME, "Could not write to runlog file: %s",
		    logfile);
	}
	misc_cloexec(fileno(fd));

	/* Increase chance that output reaches log file despite fatal errors. */
	setlinebuf(fd);

	gettimeofday(&now, NULL);
	fprintf(fd, "Run-log for %s started at %s\n", exec, _fmt_time(&now));

	runlog->fd = fd;
	runlog->start = now;
	runlog->exec = misc_strdup(exec);
}

static void runlog_finalize(struct runlog_data *log, int status)
{
	struct timeval now;

	if (!log->fd)
		return;

	if (WIFEXITED(status))
		runlog_printf(log, "exit with code %d\n", WEXITSTATUS(status));
	if (WIFSIGNALED(status))
		runlog_printf(log, "killed by signal %d\n", WTERMSIG(status));

	gettimeofday(&now, NULL);
	fprintf(log->fd, "Run-log for %s stopped at %s\n\n", log->exec,
		_fmt_time(&now));
}

static void runlog_close(struct runlog_data *log)
{
	if (log->fd)
		fclose(log->fd);
	free(log->exec);
}

/* Filter each line of testexec output. */
static void run_handler(void *data, char *line, struct rec_stream *stream)
{
	struct run_data *d = data;

	runlog_printf(&d->runlog, "%s: %s", stream->name, line);

	if (!d->check_done) {
		if (strcmp(stream->name, "stdout") == 0 &&
		    strncmp(line, "TAP ", 4) == 0)
			d->is_tap13 = true;
		d->check_done = true;
	}

	if (d->is_tap13)
		handle_tap_line(d, line, stream);
	else
		handle_nontap_line(d, line, stream);
}

static void read_file_to_env(char ***env_ptr, char *filename)
{
	char **env, *line = NULL, *value;
	FILE *file;
	size_t n;
	int num;

	env = misc_malloc(sizeof(char *));
	num = 1;

	file = fopen(filename, "r");
	if (!file)
		err(EXIT_RUNTIME, "Could not open file '%s'", filename);

	while (getline(&line, &n, file) != -1) {
		value = strchr(line, '=');
		if (!value)
			continue;

		*value = 0;
		value++;

		misc_strip_space(value);
		misc_unquote(value, NULL, shell_unescape_double_map);
		misc_add_one_env(&env, &num, line, value);
	}
	free(line);

	fclose(file);

	*env_ptr = env;
}

/* Initialize @data for use in per-line output handlers. */
static char *prepare_data(struct run_data *data, char *exec, char *matchenv,
			  char *matcherr)
{
	char *reason = NULL, *reqfile, *resfile, *v;
	struct yaml_node *yaml;
	struct config_t cfg;

	memset(data, 0, sizeof(*data));

	/* Prepare run-time data-> */
	data->exec = misc_abspath(exec);
	if (!data->exec) {
		err(EXIT_RUNTIME, "Could not determine path to command '%s'",
		    exec);
	}
	data->exec_dir = misc_dirname(data->exec);
	data->rexec = misc_relpath(data->exec, NULL);

	/* Handle testexec YAML file. */
	reqfile = misc_asprintf("%s.yaml", data->exec);

	/* Get test configuration. */
	yaml = yaml_parse_file(reqfile);
	config_parse(&cfg, yaml);
	data->plan = cfg.plan;
	data->large_temp = cfg.large_temp;
	data->desc = cfg.desc;
	yaml_free(yaml);

	/* Get environment variables describing requested resources. */
	if (matcherr) {
		reason = misc_strdup(matcherr);
	} else if (matchenv) {
		read_file_to_env(&data->env, matchenv);
	} else {
		resfile = res_get_resource_path();
		data->env = res_resolve(reqfile, resfile, true, true,
					&reason, &data->matchfile);
		free(resfile);
	}

	free(reqfile);

	v = getenv("TELA_RUNLOG");
	if (v && *v)
		runlog_open(&data->runlog, v, data->exec);

	return reason;
}

static void cleanup_data(struct run_data *data)
{
	int i;

	free(data->exec);
	free(data->exec_dir);
	free(data->last_stderr);
	if (data->env) {
		for (i = 0; data->env[i]; i++)
			free(data->env[i]);
		free(data->env);
	}
	if (data->matchfile) {
		misc_remove(data->matchfile);
		free(data->matchfile);
	}
	runlog_close(&data->runlog);
}

static void finish_tap(struct run_data *data, struct rec_result *res)
{
	if (WIFSIGNALED(res->status)) {
		twarn(data->exec, 0, "Test executable was killed by "
		      "signal %d\n", WTERMSIG(res->status));
	}
}

static void finish_nontap(struct run_data *data, struct rec_result *res)
{
	enum tela_result_t result = TELA_FAIL;

	data->num = 1;

	/* Create output for test result. */
	if (WIFEXITED(res->status)) {
		switch (WEXITSTATUS(res->status)) {
		case 0:
			result = TELA_PASS;
			break;
		case 2:
			result = TELA_SKIP;
			break;
		case 3:
			result = TELA_TODO;
			break;
		default:
			break;
		}
	}

	/* Use last stderr line only for skip and todo reasons. */
	if (result != TELA_SKIP && result != TELA_TODO) {
		free(data->last_stderr);
		data->last_stderr = NULL;
	} else if (data->last_stderr)
		misc_strip_space(data->last_stderr);

	log_plan(stdout, data->plan);
	log_result(stdout, data->rexec, data->exec, data->num, result,
		   data->last_stderr, res, data->desc, NULL);
}

static void plan_mismatch(struct run_data *data, const char *names)
{
	log_all_result(stdout, data->exec, TELA_FAIL, NULL, NULL, data->rexec,
		       data->desc, data->num, data->plan);

	if (names) {
		twarn(data->exec, 0, "Plan mismatch (missing tests:%s)\n",
		      names);
	} else {
		twarn(data->exec, 0, "Plan mismatch (plan=%d, actual=%d)\n",
		      data->plan, data->num);
	}
}

static void skip_test(struct run_data *data, const char *reason)
{
	int max;

	if (data->plan == -1)
		max = 1;
	else
		max = data->plan;

	log_plan(stdout, max);

	log_all_result(stdout, data->exec, TELA_SKIP, reason, NULL, data->rexec,
		       data->desc, 0, data->plan);
}

static void set_osid(void)
{
	FILE *fd;
	size_t n;
	char *v, id[40] = "", version[40] = "";

	/* Try to use cached value from environment. */
	v = getenv("TELA_OS");
	if (v && sscanf(v, "os: id: %40s version: %40s", id, version) == 2)
		goto out;

	/* Get value from internal os command. */
	fd = misc_internal_cmd("", "os");
	if (!fd)
		goto out;

	v = NULL;
	while (getline(&v, &n, fd) != -1) {
		if (sscanf(v, " id: %40s", id) == 1)
			continue;
		if (sscanf(v, " version: %40s", version) == 1)
			continue;
	}
	free(v);
	pclose(fd);

out:
	if (*id && *version) {
		setenv("TELA_OS_ID", id, 1);
		setenv("TELA_OS_VERSION", version, 1);
	} else
		warnx("Could not determine OS level");
}

static void setup_env(char *tmpdir, struct run_data *data)
{
	int i;
	char *d;

	set_osid();
	setenv("TELA_TMP", tmpdir, 1);
	setenv("TELA_EXEC", data->exec, 1);
	if (data->env) {
		for (i = 0; data->env[i]; i++) {
			d = strchr(data->env[i], '=');
			if (!d)
				continue;
			*d = 0;
			setenv(data->env[i], d + 1, 1);
			*d = '=';
		}
	}
	if (data->matchfile)
		setenv("TELA_RESOURCE_FILE", data->matchfile, 1);
}

/* Run specified command and capture output. If output is in TAP13 format,
 * convert to canonical format. Otherwise generate TAP13 format from arbitrary
 * output. */
static int cmd_run(int argc, char *argv[])
{
	struct rec_result res;
	struct run_data data;
	int scope = REC_ALL;
	char *exec_argv[2], *tmpdir, *skip_reason, *names = NULL, *tmp,
	     *matchenv = NULL, *matcherr = NULL;
	struct yaml_node *node, *key;

	if (argc < 1) {
		fprintf(stderr, "Usage: %s %s <command> [<scope>] [<matchenv>] "
				"[<matcherr>]\n",
			program_invocation_short_name, CMD_RUN);
		exit(EXIT_SYNTAX);
	}
	if (argc > 1 && argv[1][0])
		scope = atoi(argv[1]);
	if (argc > 2 && argv[2][0])
		matchenv = argv[2];
	if (argc > 3 && argv[3][0])
		matcherr = argv[3];

	is_stdout_tap = true;
	setvbuf(stdout, NULL, _IONBF, 0);
	log_header(stdout);

	skip_reason = prepare_data(&data, argv[0], matchenv, matcherr);
	if (skip_reason) {
		/* Test should be skipped. */
		skip_test(&data, skip_reason);
		free(skip_reason);
		goto out;
	}

	/* Use disk-based /var/tmp instead of memory-based /tmp for tests
	 * that intend to store large files. */
	tmpdir = misc_mktempdir(data.large_temp ? "/var/tmp" : NULL);
	setup_env(tmpdir, &data);

	/* Run test executable and parse output. */
	if (chdir(data.exec_dir) == -1)
		err(1, "Could not change directory");
	exec_argv[0] = data.exec;
	exec_argv[1] = NULL;
	rec_record(&res, exec_argv[0], exec_argv, scope, run_handler, &data);

	if (data.is_tap13)
		finish_tap(&data, &res);
	else
		finish_nontap(&data, &res);

	if (data.desc) {
		yaml_for_each(node, data.desc) {
			if (!node->handled && node->type == yaml_map &&
			    node->map.key &&
			    node->map.key->type == yaml_scalar) {
				key = node->map.key;
				if (!names)
					names = misc_asprintf("");
				tmp = names;
				names = misc_asprintf("%s %s", names,
						      key->scalar.content);
				free(tmp);
			}
		}
		if (names)
			plan_mismatch(&data, names);
		else if (data.plan != -1 && data.num != data.plan)
			plan_mismatch(&data, NULL);
		free(names);
	} else if (data.plan != -1 && data.num != data.plan) {
		plan_mismatch(&data, NULL);
	}

	runlog_finalize(&data.runlog, res.status);
	rec_close(&res);
	free(tmpdir);
out:
	cleanup_data(&data);

	return 0;
}

static void emit_header(FILE *log, bool pretty)
{
	if (log)
		log_header(log);
	if (!pretty)
		log_header(stdout);
}

static void emit_plan(FILE *log, int plan, bool pretty, bool diag)
{
	/* Header and test plan. */
	if (log) {
		log_plan(log, plan);
		if (diag)
			log_diag(log);
	}
	if (pretty)
		pretty_header(plan);
	else {
		log_plan(stdout, plan);
		if (diag)
			log_diag(stdout);
	}
}

static void emit_result(FILE *log, int testnum, int numtests, char *name,
			enum tela_result_t result, char *reason, bool pretty)
{
	if (log)
		log_line(log, testnum, name, result, reason);
	if (pretty)
		pretty_result(name, testnum, numtests, result, reason);
	else
		log_line(stdout, testnum, name, result, reason);
}

static void emit_bail_out(FILE *log, char *line)
{
	char *reason = strchr(line, '!');

	if (log)
		fprintf(log, "%s", line);

	if (reason) {
		reason++;
		while (isspace(*reason))
			reason++;
	}

	if (reason && *reason)
		fprintf(stderr, "Emergency stop: %s", reason);
	else
		fprintf(stderr, "Emergency stop!\n");
}

/* Create formatted output for the TAP13 data specified by @argv[0]. */
static int cmd_format(int argc, char *argv[])
{
	enum tela_result_t result;
	char *line = NULL, *name, *reason, *v, *logfile = NULL;
	const char *warning;
	size_t n;
	FILE *fd, *log = NULL;
	int num, numtests = -1, testnum = 0, rc = 0;
	bool pretty = true, verbose = false, plan_done = false, diag = false,
	     do_sync;
	struct stats_t stats;

	memset(&stats, 0, sizeof(stats));

	if (argc < 1) {
		fprintf(stderr, "Usage: %s %s <tapfile>|- [<numtests>] "
			"[<diag>]\n", program_invocation_short_name,
			CMD_FORMAT);
		exit(EXIT_SYNTAX);
	}

	/* Open output stream. */
	if (strcmp(argv[0], "-") == 0)
		fd = stdin;
	else {
		fd = fopen(argv[0], "r");
		if (!fd) {
			err(EXIT_RUNTIME, "Could not open tapfile '%s'",
			    argv[0]);
		}
	}

	/* Ensure output reaches log file despite fatal errors. */
	setlinebuf(fd);

	if (argc > 1) {
		numtests = atoi(argv[1]);
		stats.planned = numtests;
	}
	if (argc > 2 && atoi(argv[2]))
		diag = true;

	/*
	 * TELA_PRETTY - Define the output format
	 *   0: Canonical TAP13 format
	 *   1: Human readable output
	 */
	v = getenv("TELA_PRETTY");
	if (v && *v)
		pretty = atoi(v);

	/*
	 * TELA_VERBOSE - Specify output verbosity level for human readable
	 * output
	 *   0: Overview only
	 *   1: Overview + extra information
	 */
	v = getenv("TELA_VERBOSE");
	if (v && *v)
		verbose = atoi(v);

	/*
	 * TELA_WRITELOG - Filename for storing a copy of the canonical TAP13
	 * log
	 */
	logfile = getenv("TELA_WRITELOG");
	if (logfile && *logfile) {
		log = fopen(logfile, "w");
		if (!log) {
			err(EXIT_RUNTIME, "Could not open logfile '%s'",
			    logfile);
		}
		/* Ensure output reaches log file despite fatal errors. */
		setlinebuf(log);
	}

	/* Print header information. */
	emit_header(log, pretty);

	while (getline(&line, &n, fd) != -1) {
		do_sync = false;

		if (strncmp(line, "TAP ", 4) == 0) {
			/* Filter out TAP header. */
		} else if (log_parse_plan(line, &num)) {
			/* Use test plan unless specified via environment. */
			if (numtests == -1) {
				numtests = num;
				stats.planned = num;
			}
		} else if (log_parse_line(line, &v, &num, &result, &reason)) {
			/* Emit plan lazily to allow parsing of in-TAP plan. */
			if (!plan_done) {
				emit_plan(log, numtests, pretty, diag);
				plan_done = true;
			}

			/* Convert test result line to canonical form. */
			testnum++;
			if (v)
				name = misc_strdup(v);
			else {
				if (num == -1)
					num = testnum;
				name = misc_asprintf("test%d", num);
			}

			emit_result(log, testnum, numtests, name, result,
				    reason, pretty);

			free(name);
			free(v);
			free(reason);

			switch (result) {
			case TELA_PASS:
				stats.passed++;
				break;
			case TELA_SKIP:
				stats.skipped++;
				break;
			default:
				stats.failed++;
				break;
			}

			/* Sync after test result line. */
			do_sync = true;
		} else if (log_parse_bail(line)) {
			/* Terminate test run. */
			emit_bail_out(log, line);
			rc = EXIT_RUNTIME;
			break;
		} else if (strcmp(line, "# tela: query state\n") == 0) {
			if (pretty && verbose)
				printf("Collecting system state\n");
		} else {
			/* Pass anything else through. */
			if (log)
				fprintf(log, "%s", line);

			warning = log_parse_warning(line);
			if (warning) {
				stats.warnings++;
				fflush(stdout);
				fprintf(stderr, "%sWarning: %s%s",
					color_stderr.red, warning,
					color_stderr.reset);
			} else if (!pretty)
				printf("%s", line);
			else if (verbose)
				printf("%s", line);

			/* Sync at end of YAML data. */
			if (strcmp(line, "  ...\n") == 0)
				do_sync = true;
		}

		/* Make sure data reaches disk. */
		if (do_sync && log)
			fdatasync(fileno(log));
	}
	free(line);

	/* Print footer information. */
	if (pretty)
		pretty_footer(&stats, logfile);

	if (log)
		fclose(log);
	if (fd != stdin)
		fclose(fd);

	return rc;
}

/* Evaluate a single resource requirement statement. */
static int cmd_eval(int argc, char *argv[])
{
	int result;

	if (argc != 3) {
		fprintf(stderr,
			"Usage: %s %s <type> <resource> <requirement>\n",
			program_invocation_short_name, CMD_EVAL);
		exit(EXIT_SYNTAX);
	}

	result = res_eval(argv[0], argv[2], argv[1]) ? 0 : 1;

	return result;
}

static bool yamlget_cb(struct yaml_iter *iter, void *data)
{
	char *pattern = data, *quoted;

	if (fnmatch(pattern, iter->path, FNM_PATHNAME) == 0) {
		yaml_decode_path(iter->path);
		if (iter->node->type == yaml_scalar &&
		    iter->node->scalar.content) {
			quoted = misc_replace_map(iter->node->scalar.content,
						  shell_escape_single_map);
			printf("YAMLPATH='%s' VALUE='%s' TYPE='scalar'\n",
			       iter->path, quoted);
			free(quoted);
		} else if (iter->node->type == yaml_map) {
			printf("YAMLPATH='%s' VALUE='' TYPE='map'\n",
			       iter->path);
		}
	}

	return true;
}

/* Get scalar data from a YAML file. */
static int cmd_yamlget(int argc, char *argv[])
{
	struct yaml_node *root;
	int i;

	if (argc < 2) {
		fprintf(stderr,
			"Usage: %s %s <yaml_file> <yaml_path1> [...]\n",
			program_invocation_short_name, CMD_YAMLGET);
		exit(EXIT_SYNTAX);
	}

	root = yaml_parse_file("%s", argv[0]);
	if (!root) {
		warnx("%s: Empty or non-existent file", argv[0]);
		return 1;
	}

	for (i = 1; i < argc; i++)
		yaml_traverse(&root, yamlget_cb, argv[i]);

	yaml_free(root);

	return 0;
}

/* Remove invalid characters from testname. */
static int cmd_fixname(int argc, char *argv[])
{
	if (argc != 1) {
		fprintf(stderr, "Usage: %s %s <testname>\n",
			program_invocation_short_name, CMD_FIXNAME);
		exit(EXIT_SYNTAX);
	}

	misc_fix_testname(argv[0]);
	printf("%s\n", argv[0]);

	return 0;
}

static void cat(const char *filename)
{
	FILE *fd;
	char *line = NULL;
	size_t n;

	fd = fopen(filename, "r");
	if (!fd)
		err(1, "Could not open temporary file '%s'", filename);

	while (getline(&line, &n, fd) != -1) {
		printf("%s", line);
	}
	free(line);

	fclose(fd);
}

static void usage_match(void)
{
	fprintf(stderr,
"Usage: %s %s REQFILE|- [RESFILE|-] [GETSTATE] [FMT]\n"
"\n"
"Try to find a match for resource requirements from a list of available\n"
"resources.\n"
"\n"
"If a match is found, exit with return code 0 and print resource matches\n"
"either as a list of KEY=VALUE pairs, or as YAML file, depending on the\n"
"value of FMT. Otherwise exit with return code 1 and print information about\n"
"missing resources to standard error.\n"
"\n"
"PARAMETERS\n"
"  REQ       Name of a YAML file containing testcase requirements. If\n"
"            specified as '-', requirements are read from standard input.\n"
"  RES       Optional name of a YAML file containing available resources.\n"
"            If specified as '-', resources are read from standard input.\n"
"            If not specified, resources found in ~/.telarc are used.\n"
"  GETSTATE  If specified as non-zero value, the state of each resource is\n"
"            automatically obtained before matching.\n"
"  FMT       Format of match data:\n"
"            - 0: KEY=VALUE pairs (default)\n"
"            - 1: YAML format\n",
		program_invocation_short_name, CMD_MATCH);
}

#define MATCH_FMT_ENV	0
#define MATCH_FMT_YAML	1

/* Try to match a YAML test requirements file against a YAML resource file. */
static int cmd_match(int argc, char *argv[])
{
	char *reqfile, *resfile, **env, *reason, *d, *matchfile = NULL;
	int i, fmt = MATCH_FMT_ENV;
	bool getstate = false;

	if (argc < 1 || argc > 4) {
		usage_match();
		exit(EXIT_SYNTAX);
	}

	/* Process arguments. */
	if (strcmp(argv[0], "-") == 0)
		reqfile = misc_strdup(argv[0]);
	else
		reqfile = misc_abspath(argv[0]);
	if (argc >= 2 && argv[1][0]) {
		if (strcmp(argv[0], "-") == 0 && strcmp(argv[1], "-") == 0) {
			errx(EXIT_SYNTAX, "Cannot specify both input files as "
			     "'-'");
		}
		resfile = misc_strdup(argv[1]);
	} else
		resfile = res_get_resource_path();

	if (argc >= 3)
		getstate = atoi(argv[2]);

	if (argc >= 4)
		fmt = atoi(argv[3]);

	/* Perform match. */
	is_stdout_tap = true;
	env = res_resolve(reqfile, resfile, true, getstate, &reason,
			  fmt == MATCH_FMT_YAML ? &matchfile : NULL);

	free(resfile);
	free(reqfile);

	/* Display result. */
	if (!env) {
		/* No match. */
		fprintf(stderr, "%s\n", reason);

		return 1;
	}

	/* Found match, show data. */
	switch (fmt) {
	case MATCH_FMT_ENV:
		for (i = 0; env[i]; i++) {
			/* Ensure quoting to allow output to be 'sourced' in
			 * Bash. */
			d = strchr(env[i], '=');
			if (d) {
				*d++ = 0;
				printf("%s=", env[i]);
				d = misc_replace_map(d,
						     shell_escape_double_map);
				printf("\"%s\"\n", d);
				free(d);
			}
		}
		break;
	case MATCH_FMT_YAML:
		cat(matchfile);
		break;
	default:
		break;
	}

	for (i = 0; env[i]; i++)
		free(env[i]);

	free(env);
	free(matchfile);

	return 0;
}

static char *need_env(const char *fmt, ...)
{
	char *val;

	get_varargs(fmt, key);
	val = getenv(key);
	if (!val)
		errx(EXIT_RUNTIME, "Missing %s variable", key);
	free(key);

	return val;
}

static void usage_console(void)
{
	fprintf(stderr,
		"Usage: %s %s <system> [keep_open]\n"
		"\n"
		"Open a console connection to the named remote SYSTEM. Input received on the\n"
		"standard input stream will be sent to the console as input. Console output\n"
		"will be displayed on the standard output stream. The connection is closed when\n"
		"EOF is received on standard input, when the process is killed by a signal, or\n"
		"when the console host terminates the connection.\n"
		"\n"
		"Note: The current support is limited to consoles of z/VM guests.\n"
		"\n"
		"The following internal commands are understood when received on stdin:\n"
		"\n"
		"  - #tela expect <expr>\n"
		"\n"
		"    Wait until an output line matching the specified expression EXPR is received\n"
		"    as console output. EXPR must be a valid POSIX Extended Regular Expression.\n"
		"\n"
		"  - #tela idle [<n>]\n"
		"\n"
		"    Wait until no console output has been received for at least N seconds.\n"
		"    If not specified, N is assumed to be 1 second..\n"
		"\n"
		"  - #tela timeout <n>\n"
		"\n"
		"    Specify the number of seconds after which to continue even if the condition\n"
		"    for a wait operation was not met. If N is specified as 0, timeout handling\n"
		"    is completely disabled. Default is 20 seconds.\n"
		"\n"
		"ENVIRONMENT VARIABLES\n"
		"  - TELA_SYSTEM_<system>_CONSOLE_HOST\n"
		"    Name or IP address of the host providing the console access.\n"
		"    For z/VM, this is the z/VM host name or address.\n"
		"\n"
		"  - TELA_SYSTEM_<system>_CONSOLE_USER\n"
		"    User name for console access. For z/VM, this is the guest name.\n"
		"\n"
		"  - TELA_SYSTEM_<system>_CONSOLE_PASSWORD\n"
		"    Password for console access. For z/VM this is the guest password.\n"
		"\n"
		"  - TELA_SYSTEM_<system>_HYPERVISOR_TYPE=zvm\n"
		"    Hypervisor type.\n"
		"\n"
		"PARAMETERS\n"
		"  <system>     Name of the system resource to connect to\n"
		"  <keep_open>  If non-zero, keep the console connection open after EOF on stdin\n"
		"\n"
		"EXIT CODES\n"
		"  0  Command completed successfully\n"
		"  1  There was a runtime error\n"
		"  2  Error while connecting to the console server\n"
		"  3  A timeout occurred (e.g. during a '#tela expect' command)\n",
		program_invocation_short_name, CMD_CONSOLE);
}

/* Open a connection to the console of the specified system resource.
 * Data entered on STDIN is sent to the console as input. Data written to the
 * console is written to STDOUT. */
static int cmd_console(int argc, char *argv[])
{
	char *system, *type, *host, *user, *pass;
	bool keep_open = false;
	int rc;

	if (argc < 1) {
		usage_console();
		exit(EXIT_SYNTAX);
	}

	system = argv[0];
	if (argc > 1)
		keep_open = atoi(argv[1]);

	type = need_env("TELA_SYSTEM_%s_HYPERVISOR_TYPE", system);
	host = need_env("TELA_SYSTEM_%s_CONSOLE_HOST", system);
	user = need_env("TELA_SYSTEM_%s_CONSOLE_USER", system);
	pass = need_env("TELA_SYSTEM_%s_CONSOLE_PASSWORD", system);

	if (strcmp(type, "zvm") == 0) {
		rc = cons_zvm_run(host, user, pass, keep_open);
	} else {
		errx(EXIT_RUNTIME, "Console command not available for "
		     "hypervisor type '%s'", type);
	}

	return rc;
}

/* Convert text file to indented YAML block scalar while replacing non-ASCII
 * characters with a hexadecimal representation. Write result to stdout. */
static int cmd_yamlscalar(int argc, char *argv[])
{
	bool escape = false;
	int indent = 0;
	FILE *file;

	if (argc < 1 || argc > 3) {
		fprintf(stderr,
			"Usage: %s %s <text_file>|- [<indent>] [<escape>]\n",
			program_invocation_short_name, CMD_YAMLSCALAR);
		exit(EXIT_SYNTAX);
	}

	if (strcmp(argv[0], "-") == 0) {
		file = stdin;
	} else {
		file = fopen(argv[0], "r");
		if (!file)
			err(EXIT_RUNTIME, "Could not open file '%s'", argv[0]);
	}

	if (argc > 1)
		indent = atoi(argv[1]);
	if (argc > 2)
		escape = atoi(argv[2]);

	yaml_sanitize_scalar(file, stdout, indent, escape);

	if (file != stdin)
		fclose(file);

	return 0;
}

int main(int argc, char *argv[])
{
	char *cmd;
	int rc, i;

	if (debug_level > 0) {
		debug("starting tela");
		for (i = 0; i < argc; i++)
			debug("  argv[%d]='%s'", i, argv[i]);
	}

	if (argc < 2) {
		usage();
		return 0;
	}

	cmd = argv[1];
	argc -= 2;
	argv += 2;

	if (strcmp(cmd, CMD_COUNT) == 0)
		rc = cmd_count(argc, argv);
	else if (strcmp(cmd, CMD_MONITOR) == 0)
		rc = cmd_monitor(argc, argv);
	else if (strcmp(cmd, CMD_RUN) == 0)
		rc = cmd_run(argc, argv);
	else if (strcmp(cmd, CMD_FORMAT) == 0)
		rc = cmd_format(argc, argv);
	else if (strcmp(cmd, CMD_EVAL) == 0)
		rc = cmd_eval(argc, argv);
	else if (strcmp(cmd, CMD_YAMLGET) == 0)
		rc = cmd_yamlget(argc, argv);
	else if (strcmp(cmd, CMD_FIXNAME) == 0)
		rc = cmd_fixname(argc, argv);
	else if (strcmp(cmd, CMD_MATCH) == 0)
		rc = cmd_match(argc, argv);
	else if (strcmp(cmd, CMD_CONSOLE) == 0)
		rc = cmd_console(argc, argv);
	else if (strcmp(cmd, CMD_YAMLSCALAR) == 0)
		rc = cmd_yamlscalar(argc, argv);
	else {
		usage();
		rc = EXIT_SYNTAX;
	}

	return rc;
}
