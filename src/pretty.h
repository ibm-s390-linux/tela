/* SPDX-License-Identifier: MIT */
/*
 * Functions to generate formatted output.
 *
 * Copyright IBM Corp. 2023
 */

#ifndef PRETTY_H
#define PRETTY_H

#include "log.h"
#include "misc.h"

void pretty_header(int numtests);
void pretty_result(const char *name, int num, int numtests,
		   enum tela_result_t result, const char *reason);
void pretty_footer(struct stats_t *stats, char *log);
void pretty_warn(const char *fmt, ...);

#endif /* PRETTY_H */
