/* SPDX-License-Identifier: MIT */
/*
 * Functions to produce and parse TAP13 format.
 *
 * Copyright IBM Corp. 2023
 */

#ifndef LOG_H
#define LOG_H

#include <stdbool.h>
#include <stdio.h>

#include "misc.h"
#include "yaml.h"

#define TAP13_HEADER	"TAP version 13\n"

struct rec_result;

void log_diag(FILE *log);
void log_header(FILE *fd);
void log_plan(FILE *fd, int numtests);
void log_result(FILE *fd, const char *name, const char *testexec, int num,
		enum tela_result_t result, const char *reason,
		struct rec_result *res, struct yaml_node *desc,
		const char *testrexec);
void log_all_result(FILE *fd, const char *testexec, enum tela_result_t result,
		const char *reason, struct rec_result *res,
		const char *testrexec, struct yaml_node *desc, int num,
		int plan);
bool log_parse_plan(const char *s, int *numtests);
bool log_parse_line(const char *s, char **name_p, int *num_p,
		    enum tela_result_t *result_p, char **reason_p);
bool log_parse_bail(const char *line);
const char *log_parse_warning(const char *line);
void log_line(FILE *fd, int num, const char *name, enum tela_result_t result,
	      const char *reason);

#endif /* LOG_H */
