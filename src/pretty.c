/* SPDX-License-Identifier: MIT */
/*
 * Functions to generate formatted output.
 *
 * Copyright IBM Corp. 2023
 */

#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "log.h"
#include "misc.h"
#include "pretty.h"

void pretty_header(int plan)
{
	if (plan > 0)
		printf("%sRunning %d tests%s\n", color.bold, plan, color.reset);
	else
		printf("%sRunning tests%s\n", color.bold, color.reset);
}

static void trailer(const char *col, const char *text)
{
	printf("[%s%s%s]", col, text, color.reset);
}

static int count_digits(int num)
{
	int digits;

	for (digits = 1; num >= 10; digits++)
		num /= 10;

	return digits;
}

static void print_dots(int num)
{
	while (num-- > 0)
		fputc('.', stdout);
}

static void pr_name(const char *name, int len, int numdots)
{
	printf("%s", name);

	if (len < numdots) {
		fputc(' ', stdout);
		print_dots(numdots - len - 1);
	}
}

static void pr_results(enum tela_result_t result)
{
	switch (result) {
	case TELA_PASS:
		trailer(color.green, "pass");
		break;
	case TELA_FAIL:
		trailer(color.red, "fail");
		break;
	case TELA_SKIP:
		trailer(color.blue, "skip");
		break;
	case TELA_TODO:
		trailer(color.red, "todo");
		break;
	}
}

/**
 * pretty_result - Display a formatted test result line
 * @name: test name
 * @num: number of this test
 * @plan: total number of tests
 * @result: result for this test
 * @reason: optional reason for result
 */
void pretty_result(const char *name, int num, int plan,
		   enum tela_result_t result, const char *reason)
{
	int len = strlen(name), digits, numdots;
	char *v;

	v = getenv("TELA_NUMDOTS");
	if (v)
		numdots = atoi(v);
	else
		numdots = 31;

	if (plan > 0)
		digits = count_digits(plan);
	else
		digits = -plan;

	/* Announce */
	printf("%s", color.bold);

	if (plan > 0)
		printf("(%*d/%*d) ", digits, num, digits, plan);
	else
		printf("(%*d) ", digits, num);

	printf("%s", color.reset);

	if (numdots >= 0) {
		/* (1/2) test1 ........... [skip] Missing device */
		printf("%s", color.bold);
		pr_name(name, len, numdots);
		printf("%s ", color.reset);
		pr_results(result);
		if (reason && *reason)
			printf(" %s", reason);
	} else {
		/* (1/2) [skip] test1 (Missing device) */
		pr_results(result);
		printf(" %s", color.bold);
		pr_name(name, len, numdots);
		printf("%s", color.reset);
		if (reason && *reason)
			printf(" (%s)", reason);
	}

	printf("\n");
}

void pretty_footer(struct stats_t *stats, char *log)
{
	char *logpath = NULL;
	int total, missing;

	/* Get totals. */
	total = stats->passed + stats->failed + stats->skipped;
	missing = stats->planned - total;

	/* Get test log path. */
	if (log) {
		logpath = realpath(log, NULL);
		if (!logpath)
			logpath = misc_strdup(log);
	}

	/* Print result. */
	printf("%s%d tests executed%s, ", color.bold, total, color.reset);
	if (stats->passed > 0)
		printf("%s", color.green);
	printf("%d passed%s, ", stats->passed, color.reset);
	if (stats->failed > 0 || missing > 0)
		printf("%s", color.red);
	printf("%d failed", stats->failed);
	if (missing > 0)
		printf(" + %d missing", missing);
	printf(",%s ", color.reset);
	if (stats->skipped > 0)
		printf("%s", color.blue);
	printf("%d skipped %s\n", stats->skipped, color.reset);

	if (logpath) {
		printf("Result log stored in %s\n", logpath);
		free(logpath);
	}

	if (stats->warnings == 1) {
		printf("%sNote: There was 1 warning%s\n", color.red,
		       color.reset);
	} else if (stats->warnings > 1) {
		printf("%sNote: There were %d warnings%s\n", color.red,
		       stats->warnings, color.reset);
	}
}

void pretty_warn(const char *fmt, ...)
{
	get_varargs(fmt, str);

	misc_strip_space(str);
	fprintf(stderr, "%s%s%s\n", color_stderr.red, str, color_stderr.reset);

	free(str);
}
