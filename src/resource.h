/* SPDX-License-Identifier: MIT */
/*
 * Functions to match test requirements with available resources.
 *
 * Copyright IBM Corp. 2023
 */

#ifndef RESOURCE_H
#define RESOURCE_H

#include <stdbool.h>

char *res_get_resource_path(void);
char **res_resolve(const char *reqfile, const char *resfile,
		   bool do_filter, bool do_state, char **reason_ptr,
		   char **matchfile_ptr);
bool res_eval(const char *type, const char *req, const char *res);

#endif /* RESOURCE_H */
