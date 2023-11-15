/* SPDX-License-Identifier: MIT */
/*
 * C-implementation of tela test-case API.
 *
 * Copyright IBM Corp. 2023
 */

#include <err.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "config.h"
#include "tela.h"
#include "misc.h"
#include "yaml.h"

/* Internal state. */
struct {
	bool plan_set;
	int plan;
	int testnum;
	int num_pass;
	int num_fail;
	int num_skip;
	int num_todo;
	struct timeval starttime;
	struct timeval stoptime;
	char *yaml;
	struct yaml_node *desc;
} _tap;

/* Intentionally not mentioned in tap.h - for internal use only. */
FILE *tapout;

/* registered callback function after test */
static struct {
	atresult_cb cb;
	void *data;
} _atresult;

static void plan(int numtests)
{
	fprintf(tapout, "1..%d\n", numtests);
	_tap.plan = numtests;
	_tap.plan_set = true;
}

static void print_desc_yaml(struct yaml_node *desc, const char *testname)
{
	char *v, *quoted;

	v = yaml_get_scalar(desc, testname);
	if (v) {
		quoted = yaml_quote(v);
		fprintf(tapout, "  desc: \"%s\"\n", quoted);
		free(quoted);
	}
}

/* Print additional testcase data in YAML format. */
static void print_yaml(const char *file, int line, const char *result,
		       const char *reason, const char *testname)
{
	char *rfile, *rexec;
	struct timeval duration;

	rfile = realpath(file, NULL);
	rexec = realpath(program_invocation_name, NULL);
	timersub(&_tap.stoptime, &_tap.starttime, &duration);

	fprintf(tapout, "  ---\n");
	if (_tap.yaml) {
		fprintf(tapout, "%s", _tap.yaml);
		free(_tap.yaml);
		_tap.yaml = NULL;
	}
	if (_tap.desc)
		print_desc_yaml(_tap.desc, testname);

	fprintf(tapout, "  testresult: \"%s\"\n", result);
	if (reason)
		fprintf(tapout, "  reason: \"%s\"\n", reason);
	fprintf(tapout, "  testexec: \"%s\"\n", rexec ? rexec :
		program_invocation_name);
	fprintf(tapout, "  source: \"%s:%d\"\n", rfile ? rfile : file, line);
	PRTIME(tapout, "starttime: ", &_tap.starttime, 2);
	PRTIME(tapout, "stoptime:  ", &_tap.stoptime, 2);
	PRTIME_MS(tapout, "duration_ms: ", &duration, 2);
	fprintf(tapout, "  ...\n");

	free(rexec);
	free(rfile);
}

/* Moves additional testdata from tmp_dir to the right path */
static void move_files(const char *testname)
{
	const char *tela_base = misc_framework_dir(),
		   *tela_file_archive = getenv("_TELA_FILE_ARCHIVE");
	char *tmp_dir;

	tmp_dir = misc_asprintf("%s/tela_tmp", tela_file_archive);
	if (misc_exists(tmp_dir)) {
		misc_system("%s/src/log_file.sh move_files \"%s\"",
			    tela_base, testname);
	}
	free(tmp_dir);
}

/* Executed after each test to do cleanup */
static void after_test_cleanup(const char *file, int line, const char *result,
			       const char *reason, const char *testname)
{
	print_yaml(file, line, result, reason, testname);
	if (_atresult.cb)
		_atresult.cb(testname, result, _atresult.data);
	move_files(testname);
}

void _pass(const char *file, int line, const char *name)
{
	gettimeofday(&_tap.stoptime, NULL);

	fprintf(tapout, "ok     %d - %s\n", ++_tap.testnum, name);
	after_test_cleanup(file, line, "pass", NULL, name);
	_tap.num_pass++;

	gettimeofday(&_tap.starttime, NULL);
}

void _fail(const char *file, int line, const char *name, ...)
{
	va_list ap;
	char *reason;

	va_start(ap, name);
	reason = va_arg(ap, char *);
	va_end(ap);

	gettimeofday(&_tap.stoptime, NULL);

	fprintf(tapout, "not ok %d - %s\n", ++_tap.testnum, name);
	after_test_cleanup(file, line, "fail", reason, name);
	_tap.num_fail++;

	gettimeofday(&_tap.starttime, NULL);
}

void _skip(const char *file, int line, const char *name,
	   const char *reason, ...)
{
	get_varargs(reason, str);

	gettimeofday(&_tap.stoptime, NULL);

	fprintf(tapout, "ok     %d - %s # SKIP %s\n", ++_tap.testnum, name,
		reason);
	after_test_cleanup(file, line, "skip", reason, name);
	_tap.num_skip++;
	free(str);

	gettimeofday(&_tap.starttime, NULL);
}

void _todo(const char *file, int line, const char *name,
	   const char *reason, ...)
{
	get_varargs(reason, str);

	gettimeofday(&_tap.stoptime, NULL);

	fprintf(tapout, "not ok %d - %s # TODO %s\n", ++_tap.testnum, name,
		reason);
	after_test_cleanup(file, line, "todo", reason, name);
	_tap.num_todo++;
	free(str);

	gettimeofday(&_tap.starttime, NULL);
}

bool _ok(const char *file, int line, bool cond, const char *name,
	 const char *cond_str)
{
	char *quoted;

	quoted = misc_escape(cond_str, "\"");
	yaml("ok_condition: \"%s\"", quoted);
	free(quoted);
	if (cond)
		_pass(file, line, name);
	else
		_fail(file, line, name, NULL);

	return cond;
}

void _fail_all(const char *file, int line, ...)
{
	char *s;
	int i;
	va_list ap;
	char *reason;
	struct yaml_node *node, *key;

	va_start(ap, line);
	reason = va_arg(ap, char *);
	va_end(ap);

	yaml_for_each(node, _tap.desc) {
		if (!node->handled && node->type == yaml_map) {
			key = node->map.key;
			if (key && key->type == yaml_scalar &&
			    key->scalar.content) {
				_fail(file, line, key->scalar.content, reason);
				node->handled = true;
			}
		}
	}

	for (i = _tap.testnum; i < _tap.plan; i++) {
		s = misc_asprintf("missing_name_%d", i + 1);
		_fail(file, line, s, reason);
		free(s);
	}

	exit(exit_status());
}

void _skip_all(const char *file, int line, const char *reason, ...)
{
	char *s;
	int i;
	struct yaml_node *node, *key;

	get_varargs(reason, str);

	yaml_for_each(node, _tap.desc) {
		if (!node->handled && node->type == yaml_map) {
			key = node->map.key;
			if (key && key->type == yaml_scalar &&
			    key->scalar.content) {
				_skip(file, line, key->scalar.content, str);
				node->handled = true;
			}
		}
	}

	for (i = _tap.testnum; i < _tap.plan; i++) {
		s = misc_asprintf("missing_name_%d", i + 1);
		_skip(file, line, s, str);
		free(s);
	}

	free(str);

	exit(exit_status());
}

void _bail(const char *file, int line, const char *reason, ...)
{
	get_varargs(reason, str);

	fprintf(tapout, "Bail out! %s:%d: %s\n", file, line, str);
	free(str);

	exit(EXIT_BAIL);
}

void yaml(const char *text, ...)
{
	char *y;

	get_varargs(text, str);

	misc_strip_space(str);
	y = misc_asprintf("%s  %s\n", _tap.yaml ? _tap.yaml : "", str);
	free(_tap.yaml);
	_tap.yaml = y;
	free(str);
}

void yaml_file(const char *filename, int indent, const char *key, bool escape)
{
	char *yaml = NULL, *t;
	size_t n = 0;
	FILE *file, *yamlfile;
	bool empty = false;
	struct stat buf;

	file = fopen(filename, "r");
	if (!file) {
		warn("%s: Could not open file", filename);
		return;
	}

	/* Check for empty file. */
	if (fstat(fileno(file), &buf) == -1) {
		warn("%s: Could not stat file", filename);
		goto out;
	}
	empty = (buf.st_size == 0);

	/* Create output stream in memory. */
	yamlfile = open_memstream(&yaml, &n);
	if (!yamlfile) {
		warn("Could not allocate output file for YAML data");
		goto out;
	}

	/* Add initial indentation. */
	indent += 2;

	if (key) {
		fprintf(yamlfile, "%*s%s:", indent, "", key);
		indent += 2;

		if (empty) {
			/* Use non-block scalar for empty files. */
			fprintf(yamlfile, " \"\"\n");
		} else {
			/* Start block scalar. */
			fprintf(yamlfile, " |2\n");
		}
	}

	if (!empty)
		yaml_sanitize_scalar(file, yamlfile, indent, escape);

	fclose(yamlfile);

	if (_tap.yaml) {
		/* Append to existing data. */
		t = _tap.yaml;
		_tap.yaml = misc_asprintf("%s%s", t, yaml);
		free(t);
		free(yaml);
	} else {
		_tap.yaml = yaml;
	}

out:
	fclose(file);
}

void diag(const char *text, ...)
{
	get_varargs(text, str);

	fprintf(tapout, "# %s\n", str);
	free(str);
}

int exit_status(void)
{
	/* Check for missing tests. */
	if (_tap.plan_set && _tap.testnum < _tap.plan)
		return EXIT_FAIL;

	/* Pure results. */
	if (_tap.num_pass == _tap.testnum)
		return EXIT_OK;
	if (_tap.num_fail == _tap.testnum)
		return EXIT_FAIL;
	if (_tap.num_skip == _tap.testnum)
		return EXIT_SKIP;
	if (_tap.num_todo == _tap.testnum)
		return EXIT_TODO;

	/* Mixed results */
	if (_tap.num_fail > 0 || _tap.num_todo > 0)
		return EXIT_FAIL;

	return EXIT_OK;
}

void fixname(char *testname)
{
	misc_fix_testname(testname);
}

void log_file(char *file, char *name)
{
	const char *tela_base = misc_framework_dir();
	char *file_name;

	if (name)
		file_name = misc_strdup(name);
	else
		file_name = misc_basename(file);
	misc_system("%s/src/log_file.sh log_file \"%s\" \"%s\"", tela_base,
		    file, file_name);
	free(file_name);
}

void atresult(atresult_cb cb, void *data)
{
	_atresult.cb = cb;
	_atresult.data = data;
}

static void __attribute__ ((constructor)) tap_ctr(void)
{
	struct config_t cfg;
	int tapout_fd;

	config_read(&cfg, "%s.yaml", program_invocation_name);

	/* Ensure TAP output is not affected by changes to STDOUT (close,
	 * redirect). */
	tapout_fd = dup(STDOUT_FILENO);
	if (tapout_fd == -1)
		errx(EXIT_INTERNAL, "Could not duplicate standard out");
	tapout = fdopen(tapout_fd, "w");
	if (!tapout)
		errx(EXIT_INTERNAL, "Could not duplicate standard out stream");
	setvbuf(tapout, NULL, _IONBF, 0);

	fprintf(tapout, "TAP version 13\n");
	plan(cfg.plan > 0 ? cfg.plan : 1);
	_tap.desc = cfg.desc;

	gettimeofday(&_tap.starttime, NULL);
}
