/* SPDX-License-Identifier: MIT */
/*
 * Functions to handle testcase configuration data in YAML files.
 *
 * Copyright IBM Corp. 2023
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>

struct yaml_node;

struct config_t {
	int plan;
	bool large_temp;
	struct yaml_node *desc;
};

void config_parse(struct config_t *cfg, struct yaml_node *root);
void config_read(struct config_t *cfg, const char *fmt, ...);

#endif /* CONFIG_H */
