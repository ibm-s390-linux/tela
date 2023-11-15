/* SPDX-License-Identifier: MIT */
/*
 * Functions to match test requirements with available resources.
 *
 * Copyright IBM Corp. 2023
 */

#include <ctype.h>
#include <dirent.h>
#include <err.h>
#include <fnmatch.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "misc.h"
#include "resource.h"
#include "yaml.h"

#define LOCALHOST	"localhost"
#define SYSLOCAL	"system " LOCALHOST
#define INT_PREFIX	"_tela"
#define ATTR_FINAL	INT_PREFIX "_final"
#define ATTR_ALIAS	INT_PREFIX "_alias"

typedef bool (*match_fn_t)(struct yaml_node *a, struct yaml_node *b);

/*
 * List of known resource data types and associated matching function.
 * Resource scripts can use these identifiers to associate a matching function
 * with a resource node path.
 */

static bool match_objects(struct yaml_node *a, struct yaml_node *b);
static bool match_number(struct yaml_node *a, struct yaml_node *b);
static bool match_version(struct yaml_node *a, struct yaml_node *b);
static bool match_by_type(struct yaml_node *a, struct yaml_node *b);

static struct {
	const char *name;
	match_fn_t fn;
} type_list[] = {
	{ "object",	&match_objects },
	{ "number",	&match_number },
	{ "version",	&match_version },
	{ "",		&match_by_type },
	{ NULL, NULL }
};

/*
 * List of known resource node paths and associated matching function.
 * This array is populated with the output of calling resource scripts with
 * the 'get_info' option.
 */
struct path_type_t {
	char *pattern;
	const char *type;
	match_fn_t fn;
	bool noupper;
	bool sysin;
};

struct path_type_t *path_list;
static int path_list_num;

struct match_data {
	/* YAML path to node. */
	char *path;
	struct path_type_t *path_type;

	union {
		/* For requirement nodes. */
		struct {
			/* Resource assigned to this requirement. */
			struct yaml_node **res;
			int num_res;
			/* Number of times a resource was found. */
			int num_matched;
		};

		/* For resource nodes. */
		struct {
			/* True if resource has been assigned to a
			 * requirement. */
			bool assigned;
			struct yaml_node *next_compat;
		};
	};
};

#define md(x)	((struct match_data *) (x)->data)

/*
 * List of attribute variables and their values. These can be used to correlate
 * between attribute values of different objects.
 */
struct attr_var;
struct attr_var {
	struct attr_var *next;
	char *name;
	char *value;
	struct yaml_node *req;
};

static struct attr_var *attr_vars;

static void add_attr_var(const char *name, const char *value,
			 struct yaml_node *req)
{
	struct attr_var *var;

	debug("name=%s value=%s", name, value);
	var = misc_malloc(sizeof(*var));
	var->name = misc_strdup(name);
	var->value = misc_strdup(value);
	var->req = req;
	var->next = attr_vars;
	attr_vars = var;
}

static void free_attr_var(struct attr_var *var)
{
	free(var->name);
	free(var->value);
	free(var);
}

static void del_attr_var(struct yaml_node *req)
{
	struct attr_var *var, *prev = NULL;

	for (var = attr_vars; var; var = var->next) {
		if (var->req == req)
			break;
		prev = var;
	}
	if (!var)
		return;

	debug("name=%s value=%s", var->name, var->value);
	if (prev)
		prev->next = var->next;
	else
		attr_vars = var->next;

	free_attr_var(var);
}

static char *get_attr_var_value(const char *name)
{
	struct attr_var *var;

	for (var = attr_vars; var; var = var->next) {
		if (strcmp(var->name, name) == 0)
			return var->value;
	}

	return NULL;
}

static void free_attr_vars(void)
{
	struct attr_var *var, *next;

	for (var = attr_vars; var; var = next) {
		next = var->next;
		free_attr_var(var);
	}
	attr_vars = NULL;
}

static int id_to_type_idx(const char *id)
{
	int i;

	for (i = 0; type_list[i].name; i++) {
		if (strcmp(type_list[i].name, id) == 0)
			return i;
	}

	return -1;
}

static void get_type_tags(char *tags, bool *noupper, bool *sysin)
{
	char *tag;

	if (!tags) {
		/* Use defaults. */
		*noupper = false;
		*sysin = false;
		return;
	}

	while ((tag = strsep(&tags, ","))) {
		misc_skip_space(tag);
		misc_strip_space(tag);
		if (strcmp(tag, "noupper") == 0)
			*noupper = true;
		else if (strcmp(tag, "sysin") == 0)
			*sysin = true;
	}
}

/* Populate path_list array. */
static void get_types(void)
{
	char *dir, *filename, *line = NULL, *str, *pattern, *type, *tags;
	size_t n;
	DIR *dirp;
	struct dirent *de;
	FILE *file;
	int idx;
	struct path_type_t *p;
	bool noupper, sysin;

	debug("ennumerating types");
	dir = misc_asprintf("%s/src/libexec/resources", misc_framework_dir());
	dirp = opendir(dir);
	if (!dirp)
		return;

	/* Read type information from <framework>/libexec/resources. */
	while ((de = readdir(dirp))) {
		if (!misc_ends_with(de->d_name, ".types"))
			continue;

		filename = misc_asprintf("%s/%s", dir, de->d_name);
		debug2("  reading %s", filename);

		file = fopen(filename, "r");
		if (!file)
			goto next;

		while (getline(&line, &n, file) != -1) {
			misc_strip_space(line);
			if (*line == 0 || *line == '#')
				continue;

			str = line;
			pattern = strsep(&str, ":");
			type = strsep(&str, ":");
			tags = str;

			if (!pattern || !type) {
				twarn(filename, 0, "Malformed line: %s", line);
				continue;
			}

			misc_skip_space(type);
			misc_strip_space(type);

			/* Get corresponding matching function. */
			idx = id_to_type_idx(type);
			if (idx == -1) {
				twarn(filename, 0, "Unknown data type: %s",
				      type);
				continue;
			}

			/* Parse tags. */
			get_type_tags(tags, &noupper, &sysin);

			/* Add entry to array. */
			debug2("  got pattern=%s type=%s noupper=%d sysin=%d",
			       pattern, type, noupper, sysin);
			misc_expand_array(&path_list, &path_list_num);
			p = &path_list[path_list_num - 1];
			p->pattern = misc_strdup(line);
			p->type = type_list[idx].name;
			p->fn = type_list[idx].fn;
			p->noupper = noupper;
			p->sysin = sysin;
		}

		fclose(file);
next:
		free(filename);
	}

	free(line);
	closedir(dirp);
	free(dir);

	debug("ennumerating types done");
}

static void free_types(void)
{
	int i;

	for (i = 0; i < path_list_num; i++)
		free(path_list[i].pattern);
	free(path_list);
	path_list = NULL;
	path_list_num = 0;
}

/* Return a textual path of YAML node @node with parent path @parent. */
static char *node_path(struct yaml_node *node, const char *parent)
{
	char *name;

	if (node->type == yaml_scalar)
		return misc_asprintf("%s/", parent);
	else if (node->type == yaml_seq)
		name = node->seq.content->scalar.content;
	else
		name = node->map.key->scalar.content;

	if (*parent)
		return misc_asprintf("%s/%s", parent, name);
	else
		return misc_strdup(name);
}

/* Convert spacing in @s to single blanks. Remove leading and trailing
 * spaces. */
static void sanitize_spacing(char *s)
{
	int from, to;
	char c, last_c = 0;

	for (from = to = 0; (c = s[from]); from++) {
		if (isspace(c)) {
			/* Skip duplicate and leading space. */
			if (isspace(last_c) || to == 0)
				continue;
			/* Convert to blank. */
			c = ' ';
		}

		last_c = s[to++] = c;
	}
	s[to] = 0;

	/* Remove trailing space. */
	while (to > 0 && isspace(s[to - 1]))
		s[--to] = 0;
}

/* Return true if @node is a scalar with non-empty content. */
static bool is_nonempty_scalar(struct yaml_node *node)
{
	return node && node->type == yaml_scalar &&
	       node->scalar.content && *node->scalar.content;
}

/*
 * Transform YAML document @root into a clean version to reduce complexity
 * when processing it.
 *
 * Guarantees after call:
 * - top-level only contains mappings
 * - scalars are non-empty
 * - sequence contents are non-empty scalars
 * - mapping keys are non-empty scalars
 * - mapping keys have defined word spacing
 *
 * Note: This function modifies @node and the nodes in @root. Use the
 *       returned value as new document root.
 */
static struct yaml_node *_sanitize_yaml(struct yaml_node *yaml, bool toplevel)
{
	struct yaml_node *node, *next, *prev = NULL;

	for (node = yaml; node; node = next) {
		next = node->next;

		/* Remove non-mappings at top-level. */
		if (toplevel && node->type != yaml_map)
			goto remove;

		/* Remove scalars without content. */
		if (node->type == yaml_scalar && !is_nonempty_scalar(node))
			goto remove;

		/* Remove sequences without scalar content. */
		if (node->type == yaml_seq &&
		    !is_nonempty_scalar(node->seq.content))
			goto remove;

		if (node->type == yaml_map) {
			/* Remove mappings without scalar key. */
			if (!is_nonempty_scalar(node->map.key))
				goto remove;

			/* Ensure single space between words in key scalars. */
			sanitize_spacing(node->map.key->scalar.content);

			/* Recursively sanitize child nodes. */
			node->map.value = _sanitize_yaml(node->map.value,
							 false);
		}

		prev = node;
		continue;
remove:
		if (prev)
			prev->next = next;
		else
			yaml = next;
		node->next = NULL;
		yaml_free(node);
	}

	return yaml;
}

/*
 * Transform YAML document @root into a clean version to reduce complexity
 * when processing it.
 */
static struct yaml_node *sanitize_yaml(struct yaml_node *yaml)
{
	return _sanitize_yaml(yaml, true);
}

/* Return key of mapping @node. */
static char *get_key(struct yaml_node *node)
{
	if (!node || node->type != yaml_map)
		return NULL;
	return node->map.key->scalar.content;
}

/* Return scalar value of mapping @node. */
static char *get_scalar_value(struct yaml_node *node)
{
	if (!node || node->type != yaml_map || !node->map.value ||
	    node->map.value->type != yaml_scalar)
		return NULL;

	return node->map.value->scalar.content;
}

/* Return scalar content of sequence @node. */
static char *get_scalar_seq(struct yaml_node *node)
{
	if (!node || node->type != yaml_seq || !node->seq.content ||
	    node->seq.content->type != yaml_scalar)
		return NULL;

	return node->seq.content->scalar.content;
}

/* Check if key of map node @node equals @name. */
static bool match_key(struct yaml_node *node, const char *name)
{
	char *key = get_key(node);

	return key && (strcmp(key, name) == 0);
}

/* Check if key of map node @node starts with @name. */
static bool match_type_name(struct yaml_node *node, const char *name)
{
	int len = strlen(name);
	char *key = get_key(node);

	return key && misc_starts_with(key, name) &&
	       (key[len] == 0 || key[len] == ' ');
}

/* Check if keys of @node_a and @node_b start with the same word. */
static bool _match_type(struct yaml_node *node_a, struct yaml_node *node_b)
{
	char *a = get_key(node_a), *b = get_key(node_b);
	int i;

	if (!a || !b)
		return false;

	for (i = 0; a[i] && !isspace(a[i]) && (a[i] == b[i]); i++)
		;

	if (a[i] == b[i]) {
		/* Exact match. */
		return true;
	}

	if (!a[i] && isspace(b[i])) {
		/* a is short, but b continues with space. */
		return true;
	}

	if (!b[i] && isspace(a[i])) {
		/* b is short, but a continues with space. */
		return true;
	}

	return false;
}

static bool match_type(struct yaml_node *node_a, struct yaml_node *node_b)
{
	bool result = _match_type(node_a, node_b);

	return result;
}

/* Check if @name is the name of a non-resource (aka "meta") section. */
static bool is_meta_section(struct yaml_node *node)
{
	return match_key(node, "test");
}

/* Check if @node is a wildcard requirement. */
static bool is_wildcard(struct yaml_node *node)
{
	char *key = get_key(node);

	return key && misc_ends_with(key, " *");
}

/*
 * Cleanup system sections in @root:
 * - move non-system sections below "system localhost"
 * - remove toplevel meta-sections
 * - rename "system" to "system localhost"
 */
static void cleanup_system(struct yaml_node *root)
{
	struct yaml_node *node, *next, *prev;

	prev = NULL;
	for (node = root; node; node = next) {
		next = node->next;

		if (match_key(node, "system")) {
			/* Rename "system" to "system localhost". */
			free(node->map.key->scalar.content);
			node->map.key->scalar.content = misc_strdup(SYSLOCAL);
		} else if (is_meta_section(node)) {
			/* Remove meta sections. */
			prev->next = next;
			node->next = NULL;
			yaml_free(node);
			continue;
		} else if (!match_type_name(node, "system")) {
			/* Make non-system mappings children of the
			 * "system localhost" mapping. */
			prev->next = next;
			node->next = NULL;
			root->map.value = yaml_append(root->map.value, node);
			continue;
		}

		prev = node;
	}
}

/*
 * Run the YAML resource file specified by @fmt through a filter program
 * and return resulting parsed YAML document. If @res is true, apply filtering
 * rules for resource files. Otherwise use filtering rules for testcase files.
 */
static struct yaml_node *filter_file(bool res, const char *fmt, ...)
{
	struct yaml_node *result = NULL;
	char *abs_filename;
	FILE *fd;

	get_varargs(fmt, filename);

	abs_filename = misc_abspath(filename);
	if (!abs_filename)
		goto out;

	debug("filter %s file %s", res ? "resource" : "requirements", filename);
	fd = misc_internal_cmd("", "filter %s %d", abs_filename, res);
	if (!fd)
		goto out;

	result = yaml_parse_stream(fd, filename);
	pclose(fd);
	debug("filter %s file %s done", res ? "resource" : "requirements",
	      filename);

out:
	free(abs_filename);
	free(filename);

	return result;
}

static void handle_duplicates(struct yaml_node *a, struct yaml_node *b)
{
	if (a->type == yaml_scalar && b->type == yaml_scalar) {
		/* New key replaces old key. */
		free(a->scalar.content);
		a->scalar.content = b->scalar.content;
		b->scalar.content = NULL;
	} else
		yaml_append(a, b);
}

/* Merge maps with the same name in @root and children. */
static void merge_yaml(struct yaml_node *root)
{
	struct yaml_node *a, *b, *b_prev, *b_next;

	for (a = root; a; a = a->next) {
		if (a->type != yaml_map)
			continue;

		for (b_prev = a, b = a->next; b; b = b_next) {
			b_next = b->next;

			if (b->type != yaml_map)
				continue;

			if (strcmp(get_key(a), get_key(b)) != 0) {
				b_prev = b;
				continue;
			}

			/* Append content of b to content of a. */
			if (a->map.value && b->map.value)
				handle_duplicates(a->map.value, b->map.value);
			else if (!a->map.value)
				a->map.value = b->map.value;

			/* Remove b. */
			b_prev->next = b->next;
			b->next = NULL;
			b->map.value = NULL;
			yaml_free(b);
		}

		/* Handle child mappings. */
		if (a->map.value && a->map.value->type == yaml_map)
			merge_yaml(a->map.value);
	}
}

/*
 * res_get_resource_path - Determine path to default resource file
 *
 * Return a newly allocated string containing the path to the resource file
 * or %NULL if no default resource file was found.
 */
char *res_get_resource_path(void)
{
	char *filename = NULL, *v;

	v = getenv("TELA_RC");
	if (v) {
		if (!misc_exists(v))
			err(1, "TELA_RC file '%s' does not exist", v);
		return misc_strdup(v);
	}

	v = getenv("HOME");
	if (v) {
		filename = misc_asprintf("%s/.telarc", v);
		if (!misc_exists(filename)) {
			/* Not an error. */
			free(filename);
			filename = NULL;
		}
	}

	return filename;
}

/* Get list of resources available for tests. */
static struct yaml_node *get_resources(const char *filename, bool filter)
{
	struct yaml_node *result;

	/* Add local system as it is always available. */
	result = yaml_parse_string("local system", SYSLOCAL ":");

	/* Read resource file. */
	if (filename) {
		if (filter)
			result->next = filter_file(true, filename);
		else if (strcmp(filename, "-") == 0) {
			result->next = yaml_parse_stream(stdin,
							 "standard input");
		} else
			result->next = yaml_parse_file("%s", filename);
	}

	/* Clean up resulting resource data. */
	result = sanitize_yaml(result);
	cleanup_system(result);
	merge_yaml(result);

	return result;
}

/* Get list of resources requested by tests. */
static struct yaml_node *get_requirements(const char *filename)
{
	struct yaml_node *result = NULL;
	struct stat buf;

	/* Local system is always a requirement. */
	result = yaml_parse_string("<internal>", SYSLOCAL ":");

	if (strcmp(filename, "-") == 0 || stat(filename, &buf) == 0) {
		/* Add contents from testcase YAML file. */
		result->next = filter_file(false, filename);
	}

	/* Clean up resulting resource data. */
	result = sanitize_yaml(result);
	cleanup_system(result);
	merge_yaml(result);

	return result;
}

static char *get_scalar(struct yaml_node *a)
{
	if (a && a->type == yaml_scalar)
		return a->scalar.content;
	return NULL;
}

/*
 * match_alias - check if ID matches state node alias
 *
 * System resource scripts can provide alias IDs for resource objects by
 * emitting attributes of the form:
 *
 *   _tela_alias: <id>
 *
 * or
 *
 *   _tela_alias:
 *     - <id1>
 *     - <id2>
 *     ...
 *
 * Resource objects with IDs matching an alias are considered equivalent for
 * the purpose of identifying unavailable resources.
 *
 * Return %true if @snode contains an alias attributes and @id matches any of
 * the listed aliases, %false otherwise.
 */
static bool match_alias(struct yaml_node *snode, const char *id)
{
	struct yaml_node *attr, *seq;
	char *key, *val;

	/* Skip to object ID. */
	while (*id && !isspace(*id))
		id++;
	misc_skip_space(id);

	/* Process alias attributes. */
	yaml_for_each(attr, snode->map.value) {
		key = get_key(attr);
		if (!key || strcmp(key, ATTR_ALIAS) != 0)
			continue;

		/* Single scalar alias. */
		val = get_scalar_value(attr);
		if (val && strcmp(val, id) == 0)
			return true;

		if (attr->map.value->type != yaml_seq)
			continue;

		/* Sequence of scalar aliases. */
		yaml_for_each(seq, attr->map.value) {
			val = get_scalar_seq(seq);
			if (val && strcmp(val, id) == 0)
				return true;
		}
	}

	return false;
}

static bool is_object(const char *path);

static bool is_resfail(void)
{
	char *v;

	v = getenv("TELA_RESFAIL");
	return v && atoi(v);
}

static void merge_state(struct yaml_node *state, struct yaml_node *res,
			char *path)
{
	struct yaml_node *rnode, *snode;
	char *cpath, *rkey;
	bool match_found;

	if (!state)
		return;

	/*
	 * Check each node in @res for a corresponding node in @state.
	 * If node is missing in state, depending on node type:
	 *  - warn about missing resource
	 *  - add data to state
	 */
	yaml_for_each(rnode, res) {
		if (rnode->type != yaml_map) {
			/* Node is of wrong type. */
			twarn(rnode->filename, rnode->lineno,
			      "Mapping expected");
			continue;
		}

		/* Find node with matching name in state. */
		rkey = rnode->map.key->scalar.content;
		match_found = false;
		yaml_for_each(snode, state) {
			if (match_key(snode, rkey)) {
				match_found = true;
				break;
			}
			if (match_alias(snode, rkey)) {
				match_found = true;
				break;
			}
		}

		cpath = node_path(rnode, path);

		if (match_found) {
			/* Resource was specified and exists. */
			if (rnode->map.value) {
				if (snode->map.value &&
				    rnode->map.value->type == yaml_map &&
				    snode->map.value->type == yaml_map) {
					/* Process child nodes. */
					merge_state(snode->map.value,
						    rnode->map.value, cpath);
				} else {
					/*
					 * Allow override of state values with
					 * values from telarc file.
					 */
					verb("%s:%d: Override %s\n",
					     rnode->filename, rnode->lineno,
					     cpath);
					yaml_free(snode->map.value);
					snode->map.value =
						yaml_dup(rnode->map.value,
							 false, false);
				}
			}
		} else if (!is_object(cpath)) {
			/* Resource was specified, but doesn't exist and
			 * is not a resource object. */
			yaml_append(state, yaml_dup(rnode, true, false));
		} else {
			/* Resource was specified, but doesn't exist. */
			twarn(rnode->filename, rnode->lineno,
			      "Resource unavailable: %s", rkey);
			if (is_resfail())
				exit(EXIT_RUNTIME);
		}

		free(cpath);
	}
}

#define COPY_MARKER	"_tela_copy "

static bool resolve_copy_cb(struct yaml_iter *iter, void *data)
{
	struct yaml_node *node = iter->node, *source, *replacement;
	char *val, *raw_path, *path;

	/* Check for scalar marker. */
	val = get_scalar_value(node);
	if (!val || !misc_starts_with(val, COPY_MARKER))
		return true;

	/* Get YAML path to referenced node. */
	raw_path = misc_asprintf("%s/%s", iter->path,
				 val + sizeof(COPY_MARKER) - 1);
	path = yaml_canon_path(raw_path);
	free(raw_path);

	/* Find node. */
	source = yaml_get_node(iter->root, path);
	if (!source) {
		yaml_decode_path(path);
		twarn(node->filename, node->lineno,
		      "Unresolved copy source '%s'", path);
	} else {
		/* Replace reference with source data. */
		replacement = yaml_dup(source, !misc_ends_with(path, "/"),
				       false);
		yaml_iter_replace(iter, replacement);
	}

	free(path);

	return true;
}

static bool check_copy_cb(struct yaml_iter *iter, void *data)
{
	char *val;

	/* Check for scalar marker. */
	val = get_scalar_value(iter->node);
	if (!val || !misc_starts_with(val, COPY_MARKER))
		return true;

	yaml_decode_path(iter->path);
	twarn(iter->node->filename, iter->node->lineno,
	      "Unresolved copy reference '%s'", iter->path);
	yaml_iter_del(iter);

	return true;
}

/* Replace _tela_copy markers in @state with data from referenced nodes. */
static struct yaml_node *resolve_copy(struct yaml_node *root)
{
	yaml_traverse(&root, resolve_copy_cb, NULL);
	yaml_traverse(&root, check_copy_cb, NULL);

	return root;
}

static bool remove_internal_cb(struct yaml_iter *iter, void *data)
{
	char *key;

	key = get_key(iter->node);
	if (key && misc_starts_with(key, INT_PREFIX))
		yaml_iter_del(iter);

	return true;
}

/* Remove internal attributes and objects named _tela. */
static struct yaml_node *remove_internal(struct yaml_node *root)
{
	yaml_traverse(&root, remove_internal_cb, NULL);

	return root;
}

/**
 * struct sysin_link_data - Preprocessing data for nodes in test req
 * @neighbor: sysin node with the same path as the containing node in test req
 * @parent: Parent node
 * @required: Flag indicating if node is required in sysin document
 */
struct sysin_link_data {
	struct yaml_node *parent;
	struct yaml_node *neighbor;
	bool required;
};

/* Check if YAML path matches pattern for sysin attributes. */
static bool is_sysin(const char *path)
{
	int i;

	for (i = 0; i < path_list_num; i++) {
		if (!path_list[i].sysin)
			continue;

		if (fnmatch(path_list[i].pattern, path, FNM_PATHNAME) == 0)
			return true;
	}

	return false;
}

/* Marker value indicating that a resource node is not required in sysin. */
#define	MARK_REMOVE	((void *) 1)

/*
 * Preprocess sysin document A and test requirement document B:
 *   - link nodes in B to their parent
 *   - link nodes in B to neighbors (node with same path) in A
 *   - mark all sysin nodes and parents in B as required nodes
 *   - mark all sysin nodes in A for removal
 */
static bool sysin_pre_cb(struct yaml_iter *a, struct yaml_iter *b, void *data)
{
	bool sysin = is_sysin(a ? a->path : b->path);
	struct sysin_link_data *link = NULL;

	if (a) {
		if (sysin) {
			/* Data will be added via merge_state(). */
			a->node->data = MARK_REMOVE;
		}
	}

	if (b) {
		b->node->data = link = misc_malloc(sizeof(*link));
		link->parent = b->parent;
		if (a)
			link->neighbor = a->node;
		else if (sysin) {
			/* Node is required in sysin. */
			while (link) {
				link->required = true;
				link = link->parent ? link->parent->data : NULL;
			}
		}
	}

	return true;
}

/* Return the neighbor of test requirement node @node. A neighbor node is a
 * node in the sysin document that has the same YAML path as @node. If no such
 * node exists, create the node (and any parents if necessary). */
static struct yaml_node *get_neighbor(struct yaml_node *node)
{
	struct sysin_link_data *link;
	struct yaml_node *parent, *copy;

	if (!node)
		return NULL;

	link = node->data;
	if (link->neighbor)
		return link->neighbor;

	/* Create new neighbor node. */
	parent = get_neighbor(link->parent);
	if (!parent) {
		/* No common root node found. */
		return NULL;
	}

	copy = yaml_dup(node, true, true);
	yaml_append_child(parent, copy);
	link->neighbor = copy;

	return copy;
}

/* Copy all required sysin nodes from @req to @sysin. */
static bool sysin_add_cb(struct yaml_iter *iter, void *data)
{
	struct sysin_link_data *link = iter->node->data;
	struct yaml_node *sysin = data, *parent = NULL, *copy;

	if (!link->required || link->neighbor)
		return true;

	copy = yaml_dup(iter->node, true, true);
	parent = get_neighbor(link->parent);
	if (parent)
		yaml_append_child(parent, copy);
	else
		yaml_append(sysin, copy);
	link->neighbor = copy;

	return true;
}

/* Remove all nodes in @sysin that are marked to be removed. */
static bool sysin_rem_cb(struct yaml_iter *iter, void *data)
{
	if (iter->node->data == MARK_REMOVE)
		yaml_iter_del(iter);

	return true;
}

static char *get_sysname(struct yaml_node *node)
{
	char *name;

	name = get_key(node);
	if (!name)
		return NULL;
	name = strchr(name, ' ');
	if (!name)
		return NULL;

	return ++name;
}

static void rename_systems(struct yaml_node *root, const char *sysname)
{
	struct yaml_node *node;

	yaml_for_each(node, root) {
		free(node->map.key->scalar.content);
		node->map.key->scalar.content =
			misc_asprintf("system %s", sysname);
	}
}

/*
 * Return a new YAML document that is intended to be passed as input to the
 * system resource script. This "sysin" data is the result of combining the
 * list of available resources from @res plus the list of test-specific
 * requirements (such as installed packages) from @req.
 *
 * Attributes that are present in both @res and @req are not included as any
 * output the system script may produce for these will be replaced by the
 * merge_state() function.
 */
static struct yaml_node *get_sysin(struct yaml_node *res, struct yaml_node *req)
{
	struct yaml_node *sysin, *req_copy;
	bool local = false;
	char *sysname;

	/* Determine if localhost processing is requested. */
	sysname = get_sysname(res);
	if (strcmp(sysname, LOCALHOST) == 0)
		local = true;

	sysin = yaml_dup(res, true, false);
	if (local) {
		/* Consider localhost requirements only. */
		req_copy = yaml_dup(req, true, false);
	} else {
		/* Consider non-localhost requirements only. */
		req_copy = yaml_dup(req->next, false, false);

		/* Combine all requirements. */
		rename_systems(req_copy, sysname);
		merge_yaml(req_copy);
	}

	/* Pre-process nodes. */
	yaml_traverse2(&sysin, &req_copy, sysin_pre_cb, NULL);

	/* Copy required sysin nodes from @req to @sysin. */
	yaml_traverse(&req_copy, sysin_add_cb, sysin);

	/* Remove marked nodes from @sysin. */
	yaml_traverse(&sysin, sysin_rem_cb, NULL);

	/* Clear + release temporary node data. */
	yaml_free_data(sysin, NULL);
	yaml_free_data(req_copy, &free);
	yaml_free(req_copy);

	return sysin;
}

static const char *get_cache_path(void)
{
	const char *v;

	/* Check environment. */
	v = getenv("TELA_CACHE");
	if (!v || atoi(v) != 1)
		return NULL;

	return getenv("_TELA_TMPDIR");
}

/*
 * The system resource script produces output dependent on:
 *   - system name
 *   - resource file
 *   - sysin test requirements attributes
 *
 * Caching works by saving previous output for each combination of the first
 * two input parameters, and checking if any of this previous output matches
 * the requested data. The resource file (res) and the output of the system
 * script (sysout) are stored in a temporary directory that persists for the
 * duration of one test run. Each combination of resource and system script
 * output file is called a "slot".
 */

/* Macro for generating path to cache file. */
#define CACHE_NAME(path, sysname, slot, suffix) \
	"%s/cache_%s_%02d_%s", (path), (sysname), (slot), (suffix)
#define CACHE_RES(path, sysname, slot) \
	CACHE_NAME((path), (sysname), (slot), "res")
#define CACHE_SYSOUT(path, sysname, slot) \
	CACHE_NAME((path), (sysname), (slot), "sysout")

/* If @res is set, find slot with same content. If @res is not set, find first
 * empty slot. */
static int find_cache_slot(const char *path, const char *sysname,
			   struct yaml_node *res)
{
	struct yaml_node *c_res;
	int i, result = -1;

	for (i = 0; result == -1; i++) {
		c_res = yaml_parse_file(CACHE_RES(path, sysname, i));

		if (res) {
			/* Look for a matching cached resource file. */
			if (!c_res)
				break;
			if (yaml_cmp(res, c_res))
				result = i;
		} else {
			/* Look for an empty cache slot. */
			if (!c_res)
				result = i;
		}

		yaml_free(c_res);
	}

	debug("sysname=%s res=%d result=%d", sysname, !!res, result);

	return result;
}

#define SYSOUT_NONE	NULL
#define SYSOUT_FAILED	((void *) 1)

static struct yaml_node *get_cached_sysout(const char *path,
					   const char *sysname,
					   struct yaml_node *res)
{
	struct yaml_node *sysout;
	int i;

	/* Find output that was generated from the same resource file. */
	i = find_cache_slot(path, sysname, res);
	if (i == -1)
		return SYSOUT_NONE;

	debug("sysout: re-using cache slot %d", i);

	sysout = yaml_parse_file(CACHE_SYSOUT(path, sysname, i));
	if (!sysout)
		return SYSOUT_FAILED;

	return sysout;
}

/* Update an existing cache entry with data for sysin attributes that was
 * first requested by the current test. */
static void update_cached_sysout(const char *path, const char *sysname,
				 struct yaml_node *res,
				 struct yaml_node *sysout)
{
	struct yaml_node *new_sysout, *old_sysout;
	int i;

	/* Find previous cache slot. */
	i = find_cache_slot(path, sysname, res);
	if (i == -1)
		return;

	debug("sysout: updating cache slot %d", i);

	/* Update cached data. */
	old_sysout = yaml_parse_file(CACHE_SYSOUT(path, sysname, i));
	new_sysout = yaml_dup(sysout, true, false);
	new_sysout = yaml_append(new_sysout, old_sysout);
	merge_yaml(new_sysout);
	yaml_write_file(new_sysout, 0, true, CACHE_SYSOUT(path, sysname, i));
	yaml_free(new_sysout);
}

/* Create a new cache entry for a resource file that was not yet used for
 * generating system resource script output. */
static void add_cached_sysout(const char *path, const char *sysname,
			      struct yaml_node *res, struct yaml_node *sysout)
{
	int i;

	/* Find empty cache slot. */
	i = find_cache_slot(path, sysname, NULL);

	debug("sysout: adding cache slot %d", i);

	/* Write resource file. */
	yaml_write_file(res, 0, true, CACHE_RES(path, sysname, i));

	/* Write sysout file. */
	yaml_write_file(sysout, 0, true, CACHE_SYSOUT(path, sysname, i));
}

static struct yaml_node *get_sysout(const char *sysname, struct yaml_node *req,
				    struct yaml_node *res)
{
	struct yaml_node *sysout, *sysin;
	const char *cache_path;
	FILE *tmpfile, *file;
	bool update = false;
	char *tmpname;

	sysin = get_sysin(res, req);

	/* Consult cache first. */
	cache_path = get_cache_path();
	if (cache_path) {
		sysout = get_cached_sysout(cache_path, sysname, res);
		if (sysout == SYSOUT_FAILED) {
			/* Data collection failed before, don't try again. */
			sysout = NULL;
			goto out;
		} else if (sysout != SYSOUT_NONE) {
			/* Check if all data required by test is available . */
			if (yaml_is_subset(sysin, sysout))
				goto out;

			/* Same resource file, but some data is missing. */
			yaml_free(sysout);
			update = true;
		}
	}

	/* Obtain state for specified system. */
	tmpfile = misc_mktempfile(&tmpname);
	yaml_write_stream(sysin, tmpfile, 0, true);
	fclose(tmpfile);

	/* Run system script. */
	debug("system %s", sysname);
	if (strcmp(sysname, LOCALHOST) == 0)
		file = misc_internal_cmd("resources", "system \"%s\"", tmpname);
	else {
		file = misc_internal_cmd("", "remote_system %s \"%s\"", sysname,
					 tmpname);
	}
	sysout = NULL;
	if (file) {
		sysout = yaml_parse_stream(file, "libexec/system output");
		pclose(file);
		if (!sysout)
			warnx("Could not get state of system %s", sysname);
	} else
		warnx("Could not get state of system %s", sysname);
	debug("system %s done", sysname);
	if (!sysout && is_resfail())
		exit(EXIT_RUNTIME);

	/* Cleanup. */
	misc_remove(tmpname);
	free(tmpname);

	/* Update cache. */
	if (cache_path) {
		if (update)
			update_cached_sysout(cache_path, sysname, res, sysout);
		else
			add_cached_sysout(cache_path, sysname, res, sysout);
	}

out:
	yaml_free(sysin);

	return sysout;
}

static pid_t start_sysout(const char *sysname, struct yaml_node *req,
			  struct yaml_node *res, const char *filename)
{
	struct yaml_node *sysout;
	pid_t pid;

	pid = fork();
	if (pid == -1)
		errx(EXIT_RUNTIME, "Could not create a new process");
	if (pid) {
		/* Parent process. */
		return pid;
	}

	/* Child process. */
	misc_flush_cleanup();
	sysout = get_sysout(sysname, req, res);
	if (sysout)
		yaml_write_file(sysout, 0, false, filename);

	exit(0);
}

static void write_text(const char *filename, const char *fmt, ...)
{
	FILE *file;

	get_varargs(fmt, text);

	file = fopen(filename, "w");
	if (file) {
		fwrite(text, 1, strlen(text), file);
		fclose(file);
	}

	free(text);
}

/* Return %true if system object @sys contains the _tela_final attribute. */
static bool is_final_sys(struct yaml_node *sys)
{
	struct yaml_node *node;
	char *key, *val;

	yaml_for_each(node, sys->map.value) {
		key = get_key(node);
		if (!key || strcmp(key, ATTR_FINAL) != 0)
			continue;

		val = get_scalar_value(node);
		if (!val || atoi(val) != 0)
			return true;
	}

	return false;
}

static struct yaml_node *get_state(struct yaml_node *req, struct yaml_node *res)
{
	struct yaml_node *result = NULL, *node, *state, *copy;
	pid_t pid, *pids = NULL;
	int num_pids = 0, i;
	char *sysname, *outdir, *outfile;

	printf("# tela: query state\n");
	fflush(stdout);

	outdir = misc_mktempdir(NULL);

	/*
	 * Perform data collection for each system in parallel to save time.
	 * The output of each process is stored in files named
	 * <outdir>/sysout.<sysname>
	 */
	yaml_for_each(node, res) {
		sysname = get_sysname(node);
		outfile = misc_asprintf("%s/sysout.%s", outdir, sysname);

		if (node != res && !req->next) {
			/*
			 * Skip data collection for remote system unless
			 * required by test. Add a dummy state to suppress
			 * "resource unavailable "messages.
			 */
			write_text(outfile, "system %s:", sysname);
		} else if (is_final_sys(node)) {
			/*
			 * A node containing the _tela_final attribute is
			 * used as-is for resource matching without data
			 * collection.
			 */
			yaml_write_file(node, 0, true, outfile);
		} else {
			/*
			 * Start sub-process for collecting data. Use a single
			 * node copy of @node here to enable use of
			 * yaml_subset() for checking cache compatibility.
			 */
			copy = yaml_dup(node, true, false);
			pid = start_sysout(sysname, req, copy, outfile);
			yaml_free(copy);

			if (pid) {
				/* Save child process PID. */
				misc_expand_array(&pids, &num_pids);
				pids[num_pids - 1] = pid;
			}
		}

		free(outfile);
	}

	/* Wait for sub-processes to complete. */
	for (i = 0; i < num_pids; i++)
		waitpid(pids[i], NULL, 0);
	free(pids);

	/* Collect resulting data files. */
	yaml_for_each(node, res) {
		sysname = get_sysname(node);

		state = yaml_parse_file("%s/sysout.%s", outdir, sysname);
		if (!state)
			continue;

		/* Add to result. */
		if (!result)
			result = state;
		else
			yaml_append(result, state);
	}

	/* Merge duplicate resource objects as may result from aliasing. */
	merge_yaml(result);

	/* Check for stale resources and merge additional data. */
	merge_state(result, res, "");
	result = resolve_copy(result);
	result = remove_internal(result);

	misc_remove(outdir);
	free(outdir);

	return result;
}

/* Assign resource node @res to requirement node @req. */
static void assign_req(struct yaml_node *req, struct yaml_node *res)
{
	struct match_data *mdata = md(req);

	debug2("assign %s => %s", md(req)->path, md(res)->path);
	misc_expand_array(&mdata->res, &mdata->num_res);
	mdata->res[mdata->num_res - 1] = res;
	md(res)->assigned = true;
}

/* Undo assignment of resource to requirement node @req. */
static void unassign_req(struct yaml_node *req)
{
	struct match_data *mdata = md(req);
	struct yaml_node *res, *node;
	int i;

	for (i = 0; i < mdata->num_res; i++) {
		res = mdata->res[i];
		debug2("unassign %s => %s", mdata->path, md(res)->path);
		md(res)->assigned = false;
	}

	free(mdata->res);
	mdata->res = NULL;
	mdata->num_res = 0;

	/* Undo attribute variable assignment. */
	del_attr_var(req);

	/* Undo assignment of child nodes. */
	if (req->type == yaml_seq) {
		for (node = req->seq.content; node; node = node->next)
			unassign_req(node);
	} else if (req->type == yaml_map) {
		for (node = req->map.value; node; node = node->next)
			unassign_req(node);
	}
}

/* Return the next object following @res that is compatible with @req. */
static struct yaml_node *next_res(struct yaml_node *res)
{
	return md(res)->next_compat;
}

static bool is_syslocal(struct yaml_node *node)
{
	char *key = get_key(node);

	return key && (strcmp(key, SYSLOCAL) == 0);
}

/* Return the first object in @res_list that is compatible with object @req. */
static struct yaml_node *first_res(struct yaml_node *res_list,
				   struct yaml_node *req)
{
	struct yaml_node *node;
	bool req_local = is_syslocal(req);

	yaml_for_each(node, res_list) {
		if (match_type(node, req) && (is_syslocal(node) == req_local))
			break;
	}

	return node;
}

/* Return the predecessor of @req in @req_list. */
static struct yaml_node *prev_req(struct yaml_node *req_list,
				  struct yaml_node *req)
{
	struct yaml_node *result;

	if (req == req_list) {
		/* No predecessor for first node. */
		return NULL;
	}

	for (result = req_list; result; result = result->next) {
		if (result->next == req)
			break;
	}

	return result;
}

static struct yaml_node *get_lowest_match(struct yaml_node *root,
					  struct yaml_node *lowest)
{
	struct yaml_node *node;

	yaml_for_each(node, root) {
		/*
		 * Disregard wildcard requirements as they match even with no
		 * assigned resource.
		 */
		if (is_wildcard(node))
			continue;

		/* Depth-first search required to find reason for non-match. */
		if (node->type == yaml_seq)
			lowest = get_lowest_match(node->seq.content, lowest);
		else if (node->type == yaml_map)
			lowest = get_lowest_match(node->map.value, lowest);

		if (!lowest || md(node)->num_matched < md(lowest)->num_matched)
			lowest = node;
	}

	return lowest;
}

/* Return a text message identifying the requirement in @req_list that had the
 * fewest matches. */
static char *reason_req(struct yaml_node *req_list)
{
	struct yaml_node *node;
	char *path, *s, *result;
	size_t len;

	node = get_lowest_match(req_list, NULL);
	path = md(node)->path;

	if (!req_list->next) {
		/* Single system requirement - shorten path for readability. */
		s = strchr(path, '/');
		if (s)
			path = s + 1;
	}

	/* Remove trailing slash. */
	s = misc_strdup(path);
	len = strlen(s);
	if (len > 0 && s[len - 1] == '/')
		s[len - 1] = 0;

	if (node->type == yaml_scalar) {
		/* Return content for simple requirements. */
		result = misc_asprintf("Missing %s: %s", s,
				       node->scalar.content);
	} else {
		/* Return path to missing object. */
		result = misc_asprintf("Missing %s", s);
	}
	free(s);

	return result;
}

enum op_t {
	op_none,
	op_ne,
	op_lt,
	op_le,
	op_gt,
	op_ge,
};

static enum op_t parse_op(char **str)
{
	char *s = *str;
	enum op_t op = op_none;

	if (s[0] == '!' && s[1] == '=') {
		s += 2;
		op = op_ne;
	} else if (s[0] == '<') {
		if (s[1] == '=') {
			s += 2;
			op = op_le;
		} else {
			s++;
			op = op_lt;
		}
	} else if (s[0] == '>') {
		if (s[1] == '=') {
			s += 2;
			op = op_ge;
		} else {
			s++;
			op = op_gt;
		}
	}

	*str = s;

	return op;
}

static bool cmp_number(long a, long b, enum op_t op)
{
	switch (op) {
	case op_none:
		return a == b;
	case op_ne:
		return a != b;
	case op_lt:
		return a < b;
	case op_le:
		return a <= b;
	case op_gt:
		return a > b;
	case op_ge:
		return a >= b;
	}

	return false;
}

/* Find @start and @end of attribute variable in @str. */
static bool scan_attr_var(char *str, char **start, char **end)
{
	char *s, *e;

	s = strstr(str, "%{");
	if (!s)
		return false;

	e = strchr(s, '}');

	*start = s;
	*end = e;

	return true;
}

/*
 * Return a newly allocated duplicate of @str where all variable occurrences
 * are replaced with their actual value. Use @node for reporting an error if
 * an unknown variable was referenced.
 */
static char *resolve_attr_var(const char *str, struct yaml_node *node)
{
	char *result, *new_result, *curr, *s, *e, *val;
	const char *reason;

	result = curr = misc_strdup(str);
	while (scan_attr_var(curr, &s, &e)) {
		if (!e) {
			reason = "unterminated variable name";
			goto err;
		}
		*s = 0;
		*e = 0;

		/* Get variable value. */
		val = get_attr_var_value(s + 2);
		if (!val) {
			reason = "undefined variable";
			goto err;
		}

		/* Replace variable reference with value. */
		new_result = misc_asprintf("%s%s%s", result, val, e + 1);

		/* Do not resolve recursive references. */
		curr = new_result + strlen(result) + strlen(val);

		free(result);
		result = new_result;
	}

	debug("'%s' => '%s'", str, result);

	return result;

err:
	free(result);
	twarn(node->filename, node->lineno, "Error in scalar: %s", reason);

	return NULL;
}

static bool assign_attr_var(char *req, char *res, struct yaml_node *node)
{
	char *copy, *s, *e;
	bool result = false;

	if (parse_op(&req) != op_none) {
		/* Attribute condition. */
		return false;
	}

	copy = misc_strdup(req);

	if (!scan_attr_var(copy, &s, &e))
		goto out;

	if (!e) {
		twarn(node->filename, node->lineno,
		      "Error in scalar: unterminated variable name");
		goto out;
	}

	*e = 0;
	if (get_attr_var_value(s + 2)) {
		/* Only first occurrence is an assignment. */
		goto out;
	}

	add_attr_var(s + 2, res, node);
	result = true;

out:
	free(copy);

	return result;
}

/*
 * Parse decimal and binary unit prefixes in @s and return the resulting
 * factor. Decimal prefixes include 'k', 'm', 'g', and 't', binary prefixes
 * include 'ki', 'mi', 'gi', 'ti'.
 */
static unsigned long parse_scale(char **s)
{
	char unit = tolower((*s)[0]), unit2 = unit ? tolower((*s)[1]) : 0;

	switch (unit) {
	case 'k':
		(*s)++;
		if (unit2 == 'i') {
			(*s)++;
			return 1UL << 10;
		}
		return 1000UL;
	case 'm':
		(*s)++;
		if (unit2 == 'i') {
			(*s)++;
			return 1UL << 20;
		}
		return 1000000UL;
	case 'g':
		(*s)++;
		if (unit2 == 'i') {
			(*s)++;
			return 1UL << 30;
		}
		return 1000000000UL;
	case 't':
		(*s)++;
		if (unit2 == 'i') {
			(*s)++;
			return 1UL << 40;
		}
		return 1000000000000UL;
	}

	return 1;
}

/*
 * Match numerical requirement:
 *   [<op>] <value> [<scale>]
 * Where <op> can be one of <, <=, >, >=, !=. Default if none is specified: ==
 * Where <scale> can be one of ki, mi, gi, ti for 2^10, 2^20, 2^30, 2^40 and
 * k, m, g, ti for 10^3, 10^6, 10^9, 10^12.
 */
static bool match_number(struct yaml_node *req_node, struct yaml_node *res_node)
{
	char *req = get_scalar(req_node), *res = get_scalar(res_node), *next,
	     *new_req = NULL;
	enum op_t op;
	long req_value, res_value;
	bool result = false;

	if (!req || !res)
		return false;

	if (assign_attr_var(req, res, req_node)) {
		/* Variable assignments are considered a match. */
		return true;
	}

	/* Resolve variable references. */
	new_req = req = resolve_attr_var(req, req_node);
	if (!req)
		goto out;

	op = parse_op(&req);

	/* Parse req. */
	req_value = strtol(req, &next, 0);
	if (next == req) {
		/* Not a number in req. */
		goto out;
	} else
		req = next;
	misc_skip_space(req);
	req_value *= parse_scale(&req);

	/* Parse res. */
	res_value = strtol(res, &next, 0);
	if (next == res) {
		/* Not a number in res. */
		goto out;
	} else
		res = next;
	misc_skip_space(res);
	res_value *= parse_scale(&res);

	result = cmp_number(res_value, req_value, op);

out:
	free(new_req);

	return result;
}

static bool cmp_string(char *a, char *b, enum op_t op)
{
	int c = strcmp(a, b);

	switch (op) {
	case op_none:
		return c == 0;
	case op_ne:
		return c != 0;
	case op_lt:
		return c < 0;
	case op_le:
		return c <= 0;
	case op_gt:
		return c > 0;
	case op_ge:
		return c >= 0;
	}

	return false;
}

static bool cmp_subver(char *a, char *b, enum op_t op)
{
	char *end;
	bool a_isstr = false, b_isstr = false;
	long a_value = 0, b_value = 0;

	if (a && *a) {
		a_value = strtol(a, &end, 10);
		if (*end) {
			a_value = 0;
			a_isstr = true;
		}
	} else
		a = "";

	if (b && *b) {
		b_value = strtol(b, &end, 10);
		if (*end) {
			b_value = 0;
			b_isstr = true;
		}
	} else
		b = "";

	if (a_isstr || b_isstr)
		return cmp_string(a, b, op);
	else
		return cmp_number(a_value, b_value, op);
}

#define	VERSION_DELIM	".-_"

/*
 * Match version numbers:
 *   [<op>] <value><delim><value><delim<...
 * Where <op> can be one of <, <=, >, >=, !=. Default if none is specified: ==
 * Where <delim> can be one of ., -, or _
 */
static bool match_version(struct yaml_node *req_node,
			  struct yaml_node *res_node)
{
	char *req = get_scalar(req_node), *res = get_scalar(res_node),
	     *new_req = NULL, *new_res = NULL, *a, *b;
	enum op_t op;
	bool result = false;

	if (!req || !res)
		return false;

	if (assign_attr_var(req, res, req_node)) {
		/* Variable assignments are considered a match. */
		return true;
	}

	/* Resolve variable references. */
	new_req = req = resolve_attr_var(req, req_node);
	if (!req)
		goto out;

	new_res = res = misc_strdup(res);
	op = parse_op(&req);

	while (req && res) {
		misc_skip_space(res);
		misc_skip_space(req);

		a = strsep(&res, VERSION_DELIM);
		b = strsep(&req, VERSION_DELIM);

		if (!cmp_subver(a, b, op_none)) {
			/* First subversion that is different. */
			result = cmp_subver(a, b, op);
			goto out;
		}
	}

	/* One version is short. */
	result = cmp_subver(res, req, op);

out:
	free(new_req);
	free(new_res);

	return result;
}

/*
 * Match scalar contents:
 *   [<op>] <value>
 * Where <op> can be !=. Default if none is specified: ==
 */
static bool match_scalar_attr(struct yaml_node *req_node,
			      struct yaml_node *res_node)
{
	char *req = get_scalar(req_node), *res = get_scalar(res_node),
	     *new_req = NULL;
	enum op_t op;
	bool result = false;

	if (!req || !res)
		return false;

	if (assign_attr_var(req, res, req_node)) {
		/* Variable assignments are considered a match. */
		return true;
	}

	/* Resolve variable references. */
	new_req = req = resolve_attr_var(req, req_node);
	if (!req)
		goto out;

	op = parse_op(&req);

	misc_skip_space(req);

	if (op == op_none)
		result = (strcmp(req, res) == 0);
	else if (op == op_ne)
		result = (strcmp(req, res) != 0);
	else {
		/* Ignore other operators. */
		twarn(req_node->filename, req_node->lineno,
		      "Operator unsupported for scalar type");
		result = (strcmp(get_scalar(req_node), res) == 0);
	}

out:
	free(new_req);

	return result;
}

static bool match_one(struct yaml_node *req, struct yaml_node *res);

static bool match_seq_attr(struct yaml_node *req, struct yaml_node *res)
{
	struct yaml_node *req_node, *res_node;

	/* Match if all attributes in sequence @req have identical
	 * counterparts in @res. */
	yaml_for_each(req_node, req) {
		yaml_for_each(res_node, res) {
			if (match_one(req_node->seq.content,
				      res_node->seq.content))
				break;
		}
		if (!res_node)
			return false;
		md(req_node)->num_matched++;
	}

	return true;
}

static bool match_by_type(struct yaml_node *a, struct yaml_node *b)
{
	enum yaml_type type = a->type;

	if (type != b->type)
		return false;

	switch (type) {
	case yaml_scalar:
		return match_scalar_attr(a, b);
	case yaml_seq:
		return match_seq_attr(a, b);
	case yaml_map:
		return match_objects(a, b);
	default:
		break;
	}

	return false;
}

#define IDX_SCALAR	0
#define IDX_SEQ		1
#define IDX_MAP		2
#define IDX_UNKNOWN	3

static bool no_match(struct yaml_node *a, struct yaml_node *b)
{
	return false;
}

static struct path_type_t *get_path_type(const char *path, enum yaml_type type)
{
	static struct path_type_t internal[4] = {
		{ NULL, "scalar", &match_scalar_attr, false, false },
		{ NULL, "seq", &match_seq_attr, false, false },
		{ NULL, "map", &match_objects, false, false },
		{ NULL, "unknown", &no_match, false, false },
	};
	struct path_type_t *result;
	int i;

	/* Find specific type. */
	for (i = 0; i < path_list_num; i++) {
		if (fnmatch(path_list[i].pattern, path, FNM_PATHNAME) == 0) {
			result = &path_list[i];
			goto out;
		}
	}

	/* Use generic type as fallback. */
	switch (type) {
	case yaml_scalar:
		result = &internal[IDX_SCALAR];
		break;
	case yaml_seq:
		result = &internal[IDX_SEQ];
		break;
	case yaml_map:
		result = &internal[IDX_MAP];
		break;
	default:
		result = &internal[IDX_UNKNOWN];
	}

out:
	debug2("%s, %d => %s", path, type, result->type);

	return result;
}

/* Determine if node with @path is an object node. */
static bool is_object(const char *path)
{
	return strcmp(get_path_type(path, yaml_map)->type, "object") == 0;
}

/* Check if resource node @res matches requirement node @req. Both nodes
 * are of the same type. */
static bool match_one(struct yaml_node *req, struct yaml_node *res)
{
	struct path_type_t *p = NULL;
	bool result;

	if (!req) {
		result = true;
		goto out;
	}
	if (!req || !res) {
		result = false;
		goto out;
	}

	/* Select appropriate matching function. */
	p = md(req)->path_type;
	result = p->fn(req, res);

out:
	debug2("cmp_%s(%s,%s)=%d\n", p ? p->type : "<none>",
	       req ? md(req)->path : "<none>",
	       res ? md(res)->path : "<none>", result);

	if (result && req)
		md(req)->num_matched++;

	return result;
}

/* Try to find a matching object in @res_list for every object in @req_list.
 * Return %true on success, %false otherwise. */
static bool match_objects(struct yaml_node *req_list,
			  struct yaml_node *res_list)
{
	struct yaml_node *res, *req;
	bool result = true;

	/* Try to find a match for each requirement object. */
	yaml_for_each(req, req_list) {
		res = first_res(res_list, req);
retry:
		if (is_wildcard(req)) {
			/*
			 * Simplify wildcard requirement matching by first
			 * fulfilling non-wildcard requirements.
			 */
			continue;
		}
		debug2("req=%s", md(req)->path);

		/* Find a free resource object that fulfills requirement. */
		for (; res; res = next_res(res)) {
			if (!md(res)->assigned &&
			    match_one(req->map.value, res->map.value))
				break;
		}

		if (res) {
			/* Found a matching object. */
			debug2("found res=%s", md(res)->path);
			assign_req(req, res);
			md(req)->num_matched++;
			continue;
		}

		/* No match - go back to previous requirement. */
		while ((req = prev_req(req_list, req)) &&
		       md(req)->num_res == 0)
			;
		if (req) {
			res = md(req)->res[0];
			debug2("backtrack to req=%s res=%s",
			       md(req)->path, md(res)->path);

			/* Look for another match. */
			unassign_req(req);
			res = next_res(res);
			goto retry;
		} else {
			/* No match after checking all combinations. */
			debug2("nothing to backtrack");
			result = false;
			break;
		}
	}

	if (!result)
		goto out;

	/* Assign remaining unassigned objects to wildcard requirements. */
	yaml_for_each(req, req_list) {
		if (!is_wildcard(req))
			continue;

		for (res = first_res(res_list, req); res; res = next_res(res)) {
			if (!md(res)->assigned &&
			    match_one(req->map.value, res->map.value)) {
				assign_req(req, res);
				md(req)->num_matched++;
			}
		}
	}

out:
	return result;
}

/* Allocate a struct match_data for each collection node's node->data in
 * @root. @path specifies the yaml path to @root. */
static void alloc_md(struct yaml_node *root, const char *path, bool res)
{
	struct yaml_node *node, *c;
	struct match_data *mdata;

	yaml_for_each(node, root) {
		mdata = node->data = misc_malloc(sizeof(struct match_data));
		mdata->path = node_path(node, path);
		mdata->path_type = get_path_type(mdata->path, node->type);

		/* Recurse to children. */
		if (node->type == yaml_map)
			alloc_md(node->map.value, mdata->path, res);
		else if (node->type == yaml_seq)
			alloc_md(node->seq.content, mdata->path, res);

		if (!res)
			continue;

		/* Determine next compatible resource in list. */
		for (c = node->next; c; c = c->next) {
			if (match_type(node, c)) {
				mdata->next_compat = c;
				break;
			}
		}
	}
}

/* Release struct match_data of a single @node. @req indicates if @node is a
 * requirement node. */
static void free_one_md(struct yaml_node *node, bool req)
{
	struct match_data *mdata = node->data;

	if (!mdata)
		return;

	node->data = NULL;
	free(mdata->path);
	if (req)
		free(mdata->res);
	free(mdata);
}

/* Release all struct match_datas of each collection node's node->data in
 * @root. @req indicates if @root is a requirement node. */
static void free_md(struct yaml_node *root, bool req)
{
	struct yaml_node *node;

	yaml_for_each(node, root) {
		free_one_md(node, req);

		if (node->type == yaml_map)
			free_md(node->map.value, req);
		else if (node->type == yaml_seq)
			free_md(node->seq.content, req);
	}
}

static const char *sys_short(const char *sys)
{
	if (strcmp(sys, SYSLOCAL) == 0)
		return "system";

	return sys;
}

/* type id/type id/type => TYPE_id_TYPE_id_TYPE */
static char *extend_prefix(const char *path, const char *value, bool upper)
{
	char *result, *v;
	int i = 0, spos;

	value = sys_short(value);

	if (*path == 0)
		v = result = misc_strdup(value);
	else {
		result = misc_asprintf("%s_%s", path, value);
		v = result + strlen(path) + 1;
	}

	/* Remove wildcard character. */
	if (misc_ends_with(result, " *"))
		result[strlen(result) - 2] = 0;

	/* Convert special characters to underscore. Also find last space. */
	spos = strlen(v);
	for (i = 0; v[i]; i++) {
		if (isspace(v[i])) {
			spos = i;
			v[i] = '_';
		} else if (!isalnum(v[i]) && v[i] != '_')
			v[i] = '_';
	}

	if (upper) {
		/* Convert type portion of key to uppercase. */
		for (i = 0; i < spos; i++)
			v[i] = toupper(v[i]);
	}

	return result;
}

static void add_env(char ***env, int *num, struct yaml_node *root,
		    const char *parent, bool req);

static void _add_env(char ***env, int *num, struct yaml_node *node,
		     const char *parent, bool req)
{
	struct match_data *mdata = md(node);
	char *prefix, *one_prefix, *id;
	bool wildcard = req && is_wildcard(node);
	int i;

	if (!req && is_object(mdata->path)) {
		/* Handled via assigned object. */
		return;
	}

	/* Get environment variable prefix for this object. */
	if (node->type == yaml_map) {
		prefix = extend_prefix(parent, node->map.key->scalar.content,
				       !mdata->path_type->noupper);
	} else
		prefix = misc_strdup(parent);

	if (req && mdata->res) {
		for (i = 0; i < mdata->num_res; i++) {
			/* Generate variable name for wildcard matches. */
			if (wildcard)
				one_prefix = misc_asprintf("%s_%d", prefix, i);
			else
				one_prefix = prefix;

			/* Emit variable for each assigned object. */
			id = get_key(mdata->res[i]);
			id = id ? strchr(id, ' ') : NULL;
			if (id) {
				misc_add_one_env(env, num, one_prefix, id + 1);
				 /* Emit variables for contents of assigned
				  * object. */
				add_env(env, num, mdata->res[i]->map.value,
					one_prefix, false);
			}

			if (wildcard)
				free(one_prefix);
		}
	}

	switch (node->type) {
	case yaml_scalar:
		if (!req)
			misc_add_one_env(env, num, prefix,
					 node->scalar.content);
		break;
	case yaml_seq:
		add_env(env, num, node->seq.content, prefix, req);
		break;
	case yaml_map:
		add_env(env, num, node->map.value, prefix, req);
		break;
	}

	free(prefix);
}

static void add_env(char ***env, int *num, struct yaml_node *root,
		    const char *parent, bool req)
{
	struct yaml_node *node;

	yaml_for_each(node, root)
		_add_env(env, num, node, parent, req);
}

/* Convert data about matched requirements in @req to NULL-terminated array
 * of environment variables. */
static char **req_to_env(struct yaml_node *req)
{
	char **env;
	int num;

	env = misc_malloc(sizeof(char *));
	num = 1;

	add_env(&env, &num, req, "TELA", true);

	return env;
}

/* Remove all resource nodes that are not assigned to a request node. */
static bool remove_unmatched_cb(struct yaml_iter *iter, void *data)
{
	struct yaml_node *node = iter->node;
	struct match_data *mdata = md(node);

	if (!mdata->assigned && is_object(iter->path)) {
		free_one_md(iter->node, false);
		yaml_iter_del(iter);
	}

	return true;
}

/* Return key for an object that matches a requirement with the specified
 * @reqkey. Also add the instance number @num for wildcard requirements. */
static char *get_reskey(const char *reqkey, int num)
{
	char *key, *key2, *c;

	key = misc_strdup(sys_short(reqkey));

	/* Check for wildcard requirement. */
	c = strrchr(key, '*');
	if (!c || c[1])
		return key;

	/* Replace wildcard character with instance number. */
	*c = 0;
	key2 = misc_asprintf("%s%d", key, num);
	free(key);

	return key2;
}

/* For each request object, change the name of assigned resource objects
 * to match the requirement object name. Also generate names for wildcard
 * objects. */
static bool update_objname_cb(struct yaml_iter *iter, void *data)
{
	struct yaml_node *node = iter->node, *res, *object_id;
	struct match_data *mdata = md(node);
	char *key, *id;
	int i;

	if (!is_object(iter->path))
		return true;

	/* Get request node key, e.g. 'dasd my_dasd' */
	key = get_key(node);
	for (i = 0; i < mdata->num_res; i++) {
		res = mdata->res[i];

		/* Get resource node key, e.g. 'dasd 0.0.1000' */
		id = get_key(res);
		id = id ? strchr(id, ' ') : NULL;

		/* Create new attribute for resource object ID. */
		if (id) {
			object_id = yaml_parse_string(__func__, "_id: %s",
						      id + 1);
			if (object_id) {
				/* Insert object_id attribute as first child. */
				object_id->next = res->map.value;
				res->map.value = object_id;
			}
		}

		free(res->map.key->scalar.content);
		res->map.key->scalar.content = get_reskey(key, i);
	}

	return true;
}

/* Convert resource nodes @res into a YAML document representing matched
 * resources for test requirements @req. */
static void res_to_match(struct yaml_node **res, struct yaml_node *req)
{
	/*
	 * @res contains all known system resources - anything that was not
	 * matched is not relevant for the test program and can be removed.
	 */
	yaml_traverse(res, remove_unmatched_cb, NULL);

	/*
	 * @req contains the object names of required resources as expected
	 * by the test program. Rename matched objects accordingly and also
	 * expand wildcard requirement names.
	 */
	yaml_traverse(&req, update_objname_cb, NULL);
}

/* Find matches for all requirement nodes in @req from the resource nodes
 * in @res. On success return %NULL and set the data field of nodes in @req
 * to the matching node in @res. Otherwise return a pointer to a text string
 * describing why a match could not be found. */
static char **match_req(struct yaml_node *req, struct yaml_node *res,
			char **reason_ptr, char **matchfile_ptr)
{
	char **env = NULL;
	FILE *fd;

	debug("match requirements");

	/* Allocate temporary data needed for matching. */
	alloc_md(req, "", false);
	alloc_md(res, "", true);

	if (match_objects(req, res)) {
		env = req_to_env(req);
		*reason_ptr = NULL;

		if (matchfile_ptr) {
			/* Generate a YAML version of matched data. */
			fd = misc_mktempfile(matchfile_ptr);
			res_to_match(&res, req);
			yaml_write_stream(res, fd, 0, false);
			fclose(fd);
		}
	} else
		*reason_ptr = reason_req(req);

	/* Release temporary matching data. */
	free_attr_vars();
	free_md(res, false);
	free_md(req, true);

	debug("match requirements done");

	return env;
}

/*
 * res_resolve - Resolve testcase resource requirements
 * @reqfile: Filename of YAML file containing testcase resource requirements
 * @resfile: Filename of YAML file containing available resources
 * @do_filter: If %true, perform filtering on @respath before reading
 * @do_state: If %true, obtain the state of the resources listed in @respath
 * @reason_ptr: Reason if requirements could not be resolved
 * @matchfile_ptr: If non-%null, write the matching resources in YAML format to
 *                 a temporary file and store its name in @matchfile_ptr
 *
 * Try to resolve all testcase resource requirements specified in @reqfile
 * with the available resources specified in @resfile.
 *
 * If successful, return a NULL-terminated array of key-value strings
 * representing data about the matching resources. These strings should be made
 * available as environment variables for test programs.
 *
 * If no match was found, return %NULL and set  @reason_ptr to a string
 * describing a reason why the requirements could not be resolved.
 */
char **res_resolve(const char *reqfile, const char *resfile,
		   bool do_filter, bool do_state, char **reason_ptr,
		   char **matchfile_ptr)
{
	struct yaml_node *res, *req, *state;
	char **env;

	get_types();

	/* Get requirements. */
	req = get_requirements(reqfile);

	/* Get list of available resources. */
	res = get_resources(resfile, do_filter);

	/* Get state of resources. */
	if (do_state)
		state = get_state(req, res);
	else
		state = yaml_dup(res, false, false);

	/* Try to find a match for all requirements. */
	env = match_req(req, state, reason_ptr, matchfile_ptr);

	/* Release temporary resources. */
	yaml_free(state);
	yaml_free(res);
	yaml_free(req);

	free_types();

	return env;
}

/**
 * res_eval - Resolve a single test case requirement
 * @type: Type identifier corresponding to type_list.name
 * @req: Requirement value and optional condition
 * @res: Resource value
 *
 * Returns %true if resource @a of type @type meets requirement @b, %false
 * otherwise.
 */
bool res_eval(const char *type, const char *req, const char *res)
{
	struct yaml_node *req_node = NULL, *res_node = NULL;
	bool result = false;
	int i;

	i = id_to_type_idx(type);
	if (i == -1) {
		warnx("Unknown resource type '%s'", type);
		fprintf(stderr, "Known types:\n");
		for (i = 0; type_list[i].name; i++)
			fprintf(stderr, "  - %s\n", type_list[i].name);
		goto out;
	}
	req_node = yaml_parse_string("cmdline", "\"%s\"", req);
	res_node = yaml_parse_string("cmdline", "\"%s\"", res);

	debug("compare type='%s' req='%s' res='%s'", type, req, res);
	result = type_list[i].fn(req_node, res_node);

out:
	yaml_free(res_node);
	yaml_free(req_node);

	return result;
}
