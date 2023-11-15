/* SPDX-License-Identifier: MIT */
/*
 * Functions to interact with consoles of remote z/VM guest systems.
 *
 * Copyright IBM Corp. 2023
 */

#ifndef CONSOLE_ZVM_H
#define CONSOLE_ZVM_H

#include <stdbool.h>

int cons_zvm_run(char *host, char *user, char *pass, bool keep_open);

#endif /* CONSOLE_ZVM_H */
