/* SPDX-License-Identifier: MIT */
/*
 * Functions to produce and parse TAP13 format.
 *
 * Copyright IBM Corp. 2023
 */

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "log.h"
#include "misc.h"
#include "record.h"
#include "yaml.h"

#define NAME_MAXLEN	256

/* Log basic system diagnostics data to @log. */
void log_diag(FILE *log)
{
	char *line = NULL;
	size_t n;
	FILE *fd;

	fd = misc_internal_cmd("", "diag");
	if (!fd)
		return;

	while (getline(&line, &n, fd) != -1)
		fprintf(log, "# %s", line);

	free(line);
	pclose(fd);
}

/* Write TAP13 header to @fd. */
void log_header(FILE *fd)
{
	fprintf(fd, "%s", TAP13_HEADER);
}

void log_plan(FILE *fd, int plan)
{
	if (plan > 0)
		fprintf(fd, "1..%d\n", plan);
}

void log_line(FILE *fd, int num, const char *name, enum tela_result_t result,
	      const char *reason)
{
	if (!name)
		name = "";
	if (!reason)
		reason = "";

	switch (result) {
	case TELA_PASS:
		fprintf(fd, "ok     %d - %s", num, name);
		if (*reason)
			fprintf(fd, " # %s", reason);
		fprintf(fd, "\n");
		break;
	case TELA_FAIL:
		fprintf(fd, "not ok %d - %s", num, name);
		if (*reason)
			fprintf(fd, " # %s", reason);
		fprintf(fd, "\n");
		break;
	case TELA_SKIP:
		fprintf(fd, "ok     %d - %s # SKIP %s\n", num, name, reason);
		break;
	case TELA_TODO:
		fprintf(fd, "not ok %d - %s # TODO %s\n", num, name, reason);
		break;
	}
}

static const char *get_result_str(enum tela_result_t result)
{
	switch (result) {
	case TELA_PASS:
		return "pass";
	case TELA_SKIP:
		return "skip";
	case TELA_TODO:
		return "todo";
	default:
		return "fail";
	}
}

void log_result(FILE *fd, const char *name, const char *testexec, int num,
		enum tela_result_t result, const char *reason,
		struct rec_result *res, struct yaml_node *desc,
		const char *testrexec)
{
	char *v, *s, *quoted;

	if (testrexec)
		s = misc_asprintf("%s:%s", testrexec, name);
	else
		s = misc_strdup(name);
	log_line(fd, num, s, result, reason);
	free(s);

	fprintf(fd, "  ---\n");
	v = yaml_get_scalar(desc, name);
	if (v) {
		quoted = yaml_quote(v);
		fprintf(fd, "  desc: \"%s\"\n", quoted);
		free(quoted);
	}
	fprintf(fd, "  testresult: \"%s\"\n", get_result_str(result));
	if (reason)
		fprintf(fd, "  reason: \"%s\"\n", reason);
	fprintf(fd, "  testexec: \"%s\"\n", testexec);
	if (res)
		rec_print(fd, res, 2);

	fprintf(fd, "  ...\n");
}

bool log_parse_plan(const char *s, int *numtests)
{
	return sscanf(s, "1..%d", numtests) == 1;
}

bool log_parse_line(const char *line, char **name_p, int *num_p,
		    enum tela_result_t *result_p, char **reason_p)
{
	char *copy, *s, *e;
	bool ok, rc = true;
	int num = -1;
	char *desc = NULL;
	char *dir = NULL;
	char *reason = NULL;
	enum tela_result_t result = TELA_FAIL;

	copy = strdup(line);
	if (!copy)
		oom();
	s = copy;

	/* ok|not ok */
	if (strncmp(s, "ok", 2) == 0) {
		ok = true;
		s += 2;
	} else if (strncmp(s, "not ok", 6) == 0) {
		ok = false;
		s += 6;
	} else {
		rc = false;
		goto out_free;
	}
	misc_skip_space(s);

	/* [<number>] */
	if (isdigit(*s)) {
		num = atoi(s);
		misc_skip_digit(s);
		misc_skip_space(s);
	}

	/* ["-"] */
	if (*s == '-') {
		s++;
		misc_skip_space(s);
	}

	/* [<description>] */
	e = strchr(s, '#');
	if (e)
		*e = 0;
	if (*s) {
		desc = s;
		misc_strip_space(desc);
	}

	/* ["#" <directive> [<reason>]] | [<reason>] */
	if (!e)
		goto out;
	s = e + 1;
	misc_skip_space(s);

	/* Check for known directives. */
	if (strncasecmp(s, "skip", 4) != 0 && strncasecmp(s, "todo", 4) != 0) {
		/* Assume reason without directive. */
		reason = s;
		misc_strip_space(reason);
		goto out;
	}

	for (e = s; *e && !isspace(*e); e++)
		;
	if (!*e)
		e = NULL;
	else
		*e = 0;
	dir = s;

	if (!e)
		goto out;
	s = e + 1;
	misc_skip_space(s);
	reason = s;
	misc_strip_space(reason);
	rc = true;

out:
	/* Determine result code. */
	if (!dir) {
		if (ok)
			result = TELA_PASS;
		else
			result = TELA_FAIL;
	} else {
		if (strcasecmp(dir, "skip") == 0)
			result = TELA_SKIP;
		else if (strcasecmp(dir, "todo") == 0)
			result = TELA_TODO;
		else {
			rc = false;
			goto out_free;
		}
	}

	/* Pass results to caller. */
	if (name_p) {
		if (desc)
			*name_p = strdup(desc);
		else
			*name_p = NULL;
	}
	if (num_p)
		*num_p = num;
	if (result_p)
		*result_p = result;
	if (reason_p) {
		if (reason)
			*reason_p = strdup(reason);
		else
			*reason_p = NULL;
	}

out_free:
	free(copy);

	return rc;
}

bool log_parse_bail(const char *line)
{
	if (misc_starts_with(line, "Bail out!"))
		return true;

	return false;
}

const char *log_parse_warning(const char *line)
{
	if (misc_starts_with(line, "# " WARN_PREFIX))
		return line + strlen("# " WARN_PREFIX) + 1;

	return NULL;
}

void log_all_result(FILE *fd, const char *testexec, enum tela_result_t result,
		    const char *reason, struct rec_result *res,
		    const char *testrexec, struct yaml_node *desc, int num,
		    int plan)
{
	int i = num;
	char *name;
	bool base;
	struct yaml_node *node, *key;

	/* Log single result with executable name in case of no plan. */
	if (plan == -1) {
		log_result(fd, testrexec, testexec, num, result,
			   reason, res, desc, NULL);
		return;
	}

	yaml_for_each(node, desc) {
		if (node->handled || node->type != yaml_map)
			continue;

		key = node->map.key;
		if (!key || key->type != yaml_scalar || !key->scalar.content)
			continue;

		name = key->scalar.content;
		base = true;

		/* Treat plan with single entry named after exec as simple
		 * test. */
		if (plan == 1 && strcmp(name, testrexec) == 0)
			base = false;

		log_result(fd, name, testexec, i + 1, result, reason, res,
			   desc, base ? testrexec : NULL);
		node->handled = true;
		i++;
	}

	for (; i < plan; i++) {
		name = misc_asprintf("missing_name_%d", i + 1);
		log_result(fd, name, testexec, i + 1, result,
			   reason, res, desc, testrexec);
		free(name);
	}
}
