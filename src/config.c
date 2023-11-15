/* SPDX-License-Identifier: MIT */
/*
 * Functions to handle testcase configuration data in YAML files.
 *
 * Copyright IBM Corp. 2023
 */

#include <err.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "config.h"
#include "misc.h"
#include "yaml.h"

static void set_defaults(struct config_t *cfg)
{
	cfg->plan = -1;
	cfg->large_temp = 0;
	cfg->desc = NULL;
}

void config_parse(struct config_t *cfg, struct yaml_node *root)
{
	struct yaml_node *plan, *node, *test;
	char *v;
	int i;

	set_defaults(cfg);
	if (!root)
		return;

	// ensure that test exists
	test = yaml_get_node(root, "test/");
	if (!test)
		return;

	plan = yaml_get_node(root, "test/plan/");

	/*
	 * plan
	 *   Number of tests implemented by testexec or testnames and desc.
	 */
	if (plan) {
		if (plan->type == yaml_scalar) {
			v = plan->scalar.content;
			cfg->plan = atoi(v);
		} else if (plan->type == yaml_map) {
			i = 0;
			yaml_for_each(node, plan)
				i++;
			cfg->plan = i;
			cfg->desc = yaml_dup(plan, false, false);
		} else {
			twarn(plan->filename, plan->lineno,
			      "Wrong type, expect either mapping or scalar");
		}
		yaml_set_handled(plan);
	} else {
		// check for empty plan
		plan = yaml_get_node(root, "test/plan");
		if (plan) {
			twarn(plan->filename, plan->lineno,
			      "Plan is defined but empty");
		}
	}

	/*
	 * large_temp: 0|1
	 *   If 1, test wants to store large amounts of data to TELA_TMP.
	 */
	v = yaml_get_scalar(root, "test/large_temp");
	if (v)
		cfg->large_temp = atoi(v);

	// test should not contain anything besides plan
	yaml_check_unhandled(test);
}

void config_read(struct config_t *cfg, const char *fmt, ...)
{
	struct yaml_node *root;

	get_varargs(fmt, filename);

	root = yaml_parse_file(filename);
	config_parse(cfg, root);
	yaml_free(root);

	free(filename);
}
