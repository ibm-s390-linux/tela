/* SPDX-License-Identifier: MIT */
/*
 * Minimal YAML-subset parser.
 *
 * Copyright IBM Corp. 2023
 *
 * Limits:
 *   - Do not support flow structures
 *   - Limit double quotes escape characters: \, \", \n
 *   - Accept single space indentation for block structures for easier
 *     writability
 */

#include <ctype.h>
#include <err.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "misc.h"
#include "yaml.h"

#define SUB_INDENT	1

struct filepos {
	const char *filename;
	FILE *fd;
	int lineno;
	bool error;
	bool eof;
	char *line;
};

static struct yaml_node *new_node(enum yaml_type type, struct filepos *pos)
{
	struct yaml_node *node;

	node = misc_malloc(sizeof(*node));
	node->type = type;
	if (pos) {
		node->filename = misc_strdup(pos->filename);
		node->lineno = pos->lineno;
	}

	return node;
}

static struct yaml_node *new_scalar(const char *str, struct filepos *pos)
{
	struct yaml_node *node = new_node(yaml_scalar, pos);

	node->scalar.content = misc_strdup(str);

	return node;
}

static struct yaml_node *get_child(struct yaml_node *node)
{
	if (node->type == yaml_map)
		return node->map.value;
	else if (node->type == yaml_seq)
		return node->seq.content;

	return NULL;
}

static void set_child(struct yaml_node *parent, struct yaml_node *node)
{
	if (parent->type == yaml_map)
		parent->map.value = node;
	else if (parent->type == yaml_seq)
		parent->seq.content = node;
}

static void append_scalar(struct yaml_node *node, char *new_s)
{
	char *old_s = node->scalar.content;

	node->scalar.content = misc_asprintf("%s %s", old_s, new_s);
	free(old_s);
}

/* Find @c in the unquoted portion of @str. Return a pointer to the first
 * occurrence of @c in @str, or %NULL if @c was not found. */
static char *strchr_unquoted(char *str, char c)
{
	char quote = 0, escape = 0;
	int i;

	for (i = 0; str[i]; i++) {
		switch (quote) {
		case 0:
			if (str[i] == c)
				return &str[i];
			if (str[i] == '"' || str[i] == '\'')
				quote = str[i];
			break;
		case '"':
			if (escape == 0 && str[i] == '\\') {
				escape = 1;
			} else if (escape == 1) {
				/* Limit: Single char escape characters only and
				 * no check for valid escape characters. */
				escape = 0;
			} else if (str[i] == '"') {
				quote = 0;
			}
			break;
		case '\'':
			if (escape == 0 && str[i] == '\\' &&
			    str[i + 1 ] == '\\')
				escape = 1;
			else if (escape == 1)
				escape = 0;
			else if (str[i] == '\'')
				quote = 0;
			break;
		}
	}

	return NULL;
}

/* Find a map key delimiter in @str. Return pointer to delimiter on success,
 * or %NULL otherwise. */
static char *find_map(char *str)
{
	for (str = strchr_unquoted(str, ':');
	     str && str[1] && !isspace(str[1]);
	     str = strchr(str + 1, ':'))
		;

	return str;
}

struct misc_map single_quote_map[] = {
	{ "''", "'" },
	{ NULL, NULL },
};

struct misc_map double_quote_map[] = {
	{ "\\\"", "\"" },
	{ "\\n", "\n" },
	{ "\\\\", "\\" },
	{ NULL, NULL },
};

static void unquote_string(struct filepos *pos, char *str)
{
	if (!misc_unquote(str, single_quote_map, double_quote_map)) {
		warnx("%s:%d: Missing closing quote", pos->filename,
		      pos->lineno);
	}
}

static struct yaml_node *_parse(struct filepos *pos, int indent);

static struct yaml_node *_parse_implicit(struct filepos *pos, int indent,
					 char *s)
{
	struct yaml_node *node = NULL;

	while (isspace(*s))
		s++;
	misc_strip_space(s);

	if (*s == 0)
		node = _parse(pos, indent + SUB_INDENT);
	else {
		unquote_string(pos, s);
		if (!pos->error)
			node = new_scalar(s, pos);
	}

	return node;
}

static const char *type_str(enum yaml_type type)
{
	switch (type) {
	case yaml_scalar:
		return "scalar";
	case yaml_seq:
		return "sequence";
	case yaml_map:
		return "mapping";
	}
	return "<unknown>";
}

static bool pos_getline(struct filepos *pos, char **line, size_t *n)
{
	size_t len;

	if (pos->error || pos->eof)
		return false;

	/* Use buffered line if available. */
	if (pos->line) {
		len = strlen(pos->line) + 1;
		if (*n  < len) {
			free(*line);
			*line = misc_malloc(len);
			*n = len;
		}
		strcpy(*line, pos->line);
		free(pos->line);
		pos->line = NULL;
		return true;
	}

	return getline(line, n, pos->fd) != -1;
}

static void pos_ungetline(struct filepos *pos, char *line)
{
	if (pos->line) {
		/* Should not happen. */
		warnx("Internal error: multiple buffered lines");
		free(pos->line);
	}

	pos->line = misc_strdup(line);
}

static struct yaml_node *_parse(struct filepos *pos, int indent)
{
	size_t n;
	char *line = NULL, *s;
	struct yaml_node *root = NULL, *node, *prev = NULL;
	int i;

	while (pos_getline(pos, &line, &n)) {
		pos->lineno++;

		debug2("%s:%d: %s", pos->filename, pos->lineno, line);

		/* Remove trailing newlines. */
		while ((s = strrchr(line, '\n')))
			*s = 0;

		/* Remove comment portions. */
		s = strchr_unquoted(line, '#');
		if (s)
			*s = 0;

		/* Skip document start marker. */
		if (misc_starts_with(line, "---"))
			continue;

		/* Document end marker. */
		if (misc_starts_with(line, "...")) {
			pos->eof = true;
			break;
		}

		/* Find indentation level. */
		for (i = 0; line[i] == ' '; i++)
			;
		s = line + i;

		/* Skip empty lines. */
		if (line[i] == 0)
			continue;

		/* Block end. */
		if (i < indent) {
			pos->lineno--;
			pos_ungetline(pos, line);
			break;
		}

		/* Check for tab indentation outside of multi-line scalar. */
		if (line[i] == '\t' && prev && prev->type != yaml_scalar) {
			twarn(pos->filename, pos->lineno,
			      "Found unsupported tab indentation");
			pos->error = true;
			break;
		}

		/* Sequence. */
		if (line[i] == '-' && isspace(line[i + 1])) {
			if (prev && prev->type != yaml_seq) {
				warnx("%s:%d: Found unexpected sequence "
				      "indicator '-' - expected %s",
				      pos->filename, pos->lineno,
				      type_str(prev->type));
				pos->error = true;
				break;
			}
			/* Extract content. */
			node = new_node(yaml_seq, pos);
			node->seq.content = _parse_implicit(pos, i,
							    line + i +
							    /* "- " */ 2);
			if (pos->error)
				break;
			goto next;
		}

		/* Mapping. */
		s = find_map(line + i);
		if (s) {
			if (prev && prev->type != yaml_map) {
				warnx("%s:%d: Found unexpected mapping "
				      "indicator ':' - expected %s",
				      pos->filename, pos->lineno,
				      type_str(prev->type));
				pos->error = true;
				break;
			}
			/* Extract key. */
			*s = 0;
			misc_strip_space(line + i);
			node = new_node(yaml_map, pos);
			node->map.key = new_scalar(line + i, pos);
			/* Extract value. */
			node->map.value = _parse_implicit(pos, i, s + 1);
			if (pos->error)
				break;
			goto next;
		}

		/* Scalar. */
		if (prev && prev->type != yaml_scalar) {
			warnx("%s:%d: Found unexpected scalar - expected %s",
			      pos->filename, pos->lineno, type_str(prev->type));
			pos->error = true;
			break;
		}

		misc_strip_space(line + i);
		unquote_string(pos, line + i);
		if (pos->error)
			break;

		if (prev)
			append_scalar(prev, line + i);
		else
			node = new_scalar(line + i, pos);

next:
		if (!root)
			root = node;
		else if (prev)
			prev->next = node;
		prev = node;
		node = NULL;
	}
	free(line);

	return root;
}

/**
 * yaml_parse_stream - Read YAML from I/O stream
 * @fd: I/O stream
 * @name: Stream name
 *
 * Read YAML from stream specified by @fd and return a newly allocated struct
 * yaml_node representing the parsed YAML content or %NULL if the file could
 * not be read or parsed.
 */
struct yaml_node *yaml_parse_stream(FILE *fd, const char *name)
{
	struct yaml_node *result;
	struct filepos pos;

	memset(&pos, 0, sizeof(pos));
	pos.filename = name;
	pos.fd = fd;

	result = _parse(&pos, 0);
	if (pos.error) {
		yaml_free(result);
		result = NULL;
	}

	return result;
}

/**
 * yaml_parse_file - Read YAML file
 * @fmt: Filename format string
 *
 * Read YAML file specified by @fmt and return a newly allocated struct
 * yaml_node representing the parsed YAML content or %NULL if the file could
 * not be read or parsed.
 */
struct yaml_node *yaml_parse_file(const char *fmt, ...)
{
	struct yaml_node *result = NULL;
	FILE *fd;

	get_varargs(fmt, filename);

	fd = fopen(filename, "r");
	if (fd) {
		result = yaml_parse_stream(fd, filename);
		fclose(fd);
	}

	free(filename);

	return result;
}

/**
 * yaml_parse_string - Parse string into YAML representation
 * @fmt: Textual YAML content
 *
 * Parse the textual YAML content @fmt and return a newly allocated struct
 * yaml_node representing the parsed YAML content or %NULL if the string could
 * not be read or parsed.
 */
struct yaml_node *yaml_parse_string(const char *name, const char *fmt, ...)
{
	struct yaml_node *result = NULL;
	FILE *fd;

	get_varargs(fmt, str);

	fd = fmemopen((void *) str, strlen(str), "r");
	if (fd) {
		result = yaml_parse_stream(fd, name);
		fclose(fd);
	}

	free(str);

	return result;
}

void yaml_free(struct yaml_node *node)
{
	struct yaml_node *next;

	if (!node)
		return;

	for (next = NULL; node; node = next) {
		free(node->filename);
		switch (node->type) {
		case yaml_scalar:
			free(node->scalar.content);
			break;
		case yaml_seq:
			yaml_free(node->seq.content);
			break;
		case yaml_map:
			yaml_free(node->map.key);
			yaml_free(node->map.value);
			break;
		}
		next = node->next;
		free(node);
	}
}

void yaml_print(struct yaml_node *node, int indent)
{
	enum yaml_type type;

	if (!node) {
		ind(indent, "~\n");
		return;
	}

	type = node->type;
	if (type == yaml_seq)
		ind(indent, "!!seq [\n");
	else if (type == yaml_map)
		ind(indent, "!!map {\n");

	for (; node; node = node->next) {
		switch (node->type) {
		case yaml_scalar:
			if (!node->scalar.content)
				ind(indent, "~\n");
			else {
				ind(indent, "!!str \"%s\"\n",
				    node->scalar.content);
			}
			break;
		case yaml_seq:
			yaml_print(node->seq.content, indent + 2);
			break;
		case yaml_map:
			ind(indent + 2, "?\n");
			yaml_print(node->map.key, indent + 4);
			ind(indent + 2, ":\n");
			yaml_print(node->map.value, indent + 4);
			break;
		}
		if (node->next)
			ind(indent + 2, ",\n");
	}

	if (type == yaml_seq)
		ind(indent, "]\n");
	else if (type == yaml_map)
		ind(indent, "}\n");
}

static int scalar_strcmp(struct yaml_node *node, const char *str)
{
	if (node->type == yaml_scalar && node->scalar.content)
		return strcmp(node->scalar.content, str);

	return -1;
}

/**
 * yaml_get_node - Get a node in a YAML document
 * @root: Root node of the YAML document
 * @path: Path to the node
 *
 * Return the node with the given document @path in the YAML document specified
 * by @root or %NULL if no such node exists. @path is a concatenation of
 * mapping keys leading to the node in question separated by '/', with slash
 * characters ('/') in mapping keys being replaced by YAML_PATH_SLASH.
 *
 * Example:
 * ---
 * a:
 *   b: "content"
 * ...
 *
 * yaml_get_node("a/b") returns the mapping node representing "b: content".
 * yaml_get_node("a/b/") returns the scalar node representing "content".
 *
 */
struct yaml_node *yaml_get_node(struct yaml_node *root, const char *path)
{
	char *copy, *str, *comp;
	struct yaml_node *node = root, *result = NULL;

	str = copy = misc_strdup(path);

	while ((comp = strsep(&str, "/"))) {
		yaml_decode_path(comp);
		for (; *comp && node; node = node->next) {
			if (node->type == yaml_map && node->map.key &&
			    scalar_strcmp(node->map.key, comp) == 0)
				break;
		}

		if (!node)
			break;
		node->handled = true;
		if (!str)
			result = node;
		else
			node = node->map.value;
	}

	free(copy);

	return result;
}

static void set_handled(struct yaml_node *node, bool value)
{
	for (; node; node = node->next) {
		node->handled = value;
		if (node->type == yaml_seq)
			set_handled(node->seq.content, value);
		else if (node->type == yaml_map)
			set_handled(node->map.value, value);
	}
}

/**
 * yaml_get_scalar - Get the contents of a scalar node in a YAML document
 * @root: Root node of the YAML document
 * @path: Path to the node
 *
 * Return the scalar content of the node with the given document @path in the
 * YAML document specified by @root or %NULL if no such node exists. @path is
 * a concatenation of mapping keys leading to the node in question.
 */
char *yaml_get_scalar(struct yaml_node *root, const char *path)
{
	struct yaml_node *node = yaml_get_node(root, path), *scalar;

	if (!node)
		return NULL;

	scalar = node->map.value;

	set_handled(scalar, true);
	if (!scalar)
		return NULL;

	if (scalar->type != yaml_scalar) {
		twarn(scalar->filename, scalar->lineno,
		      "Found %s instead of scalar\n", type_str(scalar->type));
		return NULL;
	}

	return scalar->scalar.content;
}

/**
 * yaml_check_unhandled - Check for unhandled YAML nodes
 * @root: Root node of the YAML document
 *
 * Print warnings for all nodes in the YAML document specified by @root that
 * have the handled field set to false.
 */
void yaml_check_unhandled(struct yaml_node *root)
{
	struct yaml_node *node;

	yaml_for_each(node, root) {
		debug("%s:%d: handled=%d", node->filename, node->lineno,
		      node->handled);
		if (!node->handled) {
			twarn(node->filename, node->lineno, "Unhandled %s\n",
			      type_str(node->type));
		} else if (node->type == yaml_seq && node->seq.content) {
			yaml_check_unhandled(node->seq.content);
		} else if (node->type == yaml_map && node->map.value) {
			yaml_check_unhandled(node->map.value);
		}
	}
}

/**
 * yaml_dup - Duplicate a YAML node
 * @node: Node to duplicate
 * @single: If set, do not duplicate neighbor nodes
 * @no_child: If set, do not duplicate child nodes
 *
 * Return a newly allocated YAML node which is a duplicate of @node, including
 * all content nodes.
 */
struct yaml_node *yaml_dup(struct yaml_node *node, bool single, bool no_child)
{
	struct yaml_node *res = NULL, *last, *dup;
	struct filepos pos;

	while (node) {
		/* Duplicate node and content. */
		pos.filename = node->filename;
		pos.lineno = node->lineno;
		dup = new_node(node->type, &pos);
		switch (node->type) {
		case yaml_scalar:
			if (node->scalar.content) {
				dup->scalar.content =
					misc_strdup(node->scalar.content);
			}
			break;
		case yaml_seq:
			if (no_child)
				break;
			dup->seq.content = yaml_dup(node->seq.content, false,
						    false);
			break;
		case yaml_map:
			dup->map.key = yaml_dup(node->map.key, false, false);
			if (no_child)
				break;
			dup->map.value = yaml_dup(node->map.value, false,
						  false);
			break;
		}

		/* Continue with neighbor node. */
		if (res) {
			last->next = dup;
			last = dup;
		} else {
			last = res = dup;
		}

		if (single)
			break;

		node = node->next;
	}

	return res;
}

/**
 * yaml_append - Append YAML node to end of document
 * @root: YAML document to which node should be appended to
 * @node: Node to append
 */
struct yaml_node *yaml_append(struct yaml_node *root, struct yaml_node *node)
{
	struct yaml_node *prev;

	for (prev = root; prev && prev->next; prev = prev->next)
		;

	if (prev) {
		prev->next = node;
		return root;
	}

	return node;
}

/**
 * yaml_append_child - Append YAML node to child nodes
 * @parent: Parent node
 * @node: Node to append
 */
void yaml_append_child(struct yaml_node *parent, struct yaml_node *node)
{
	set_child(parent, yaml_append(get_child(parent), node));
}

#define	STR(x)	(x) ? : ""

/* Write out a YAML node. Restrictions:
 * - sequences can only have scalar contents
 * - maps can only have scalar keys
 * - scalars can only contain non-quoted characters
 */
void _yaml_write_stream(struct yaml_node *root, FILE *file, int ind,
			bool single, bool cont)
{
	struct yaml_node *node, *c;

	yaml_for_each(node, root) {
		if (cont)
			cont = false;
		else
			fprintf(file, "%*s", ind, "");

		switch (node->type) {
		case yaml_scalar:
			fprintf(file, "%s\n", STR(node->scalar.content));
			break;
		case yaml_seq:
			c = node->seq.content;
			if (!c || c->type != yaml_scalar)
				break;
			fprintf(file, "- ");
			_yaml_write_stream(c, file, ind + 2, false, true);
			break;
		case yaml_map:
			c = node->map.key;
			if (!c || c->type != yaml_scalar)
				break;
			fprintf(file, "%s:", STR(c->scalar.content));

			c = node->map.value;
			if (!c)
				fprintf(file, "\n");
			else if (c->type == yaml_scalar)
				fprintf(file, " %s\n", STR(c->scalar.content));
			else {
				fprintf(file, "\n");
				_yaml_write_stream(c, file, ind + 2, false,
						   false);
			}
		}

		if (single)
			break;
	}
}

/**
 * yaml_write_stream - Write YAML document to I/O stream
 * @root: YAML document
 * @file: I/O stream
 * @indent: Number of blanks to use for indentation
 * @single: If %true, do not write data for neighbors of @root
 */
void yaml_write_stream(struct yaml_node *root, FILE *file, int indent,
		       bool single)
{
	_yaml_write_stream(root, file, indent, single, false);
}

/**
 * yaml_write_file - Write YAML document to file
 * @root: YAML document
 * @indent: Number of blanks to use for indentation
 * @single: If %true, do not write data for neighbors of @root
 * @fmt: Filename format string
 */
bool yaml_write_file(struct yaml_node *root, int indent, bool single,
		     const char *fmt, ...)
{
	bool result = false;
	FILE *file;

	get_varargs(fmt, filename);

	file = fopen(filename, "w");
	if (file) {
		yaml_write_stream(root, file, indent, single);
		fclose(file);
		result = true;
	}

	free(filename);

	return result;
}

static void replace_char(char *str, char from, char to)
{
	for (; *str; str++) {
		if (*str == from)
			*str = to;
	}
}

/**
 * yaml_path_decode - Turn path as returned in yaml_iter to readable format
 * @path: Path string to decode
 *
 * YAML paths that are provided to the yaml_traverse() callback are encoded so
 * that component names do not contain slash characters ('/'). This is required
 * to allow for matching path names using fnmatch. This function converts
 * such a path to normal format, e.g. for printing.
 */
void yaml_decode_path(char *path)
{
	replace_char(path, YAML_PATH_SLASH, '/');
}

/* Return a textual path of YAML node @node with parent path @parent. */
static char *node_path(struct yaml_node *node, const char *parent)
{
	char *name, *result;

	switch (node->type) {
	case yaml_scalar:
		return misc_asprintf("%s/", parent);
	case yaml_seq:
		name = node->seq.content->scalar.content;
		break;
	case yaml_map:
		name = node->map.key->scalar.content;
		break;
	}

	/* Replace '/' in name to prevent fnmatch() confusion. */
	name = misc_strdup(name);
	replace_char(name, '/', YAML_PATH_SLASH);

	if (*parent)
		result = misc_asprintf("%s/%s", parent, name);
	else
		result = misc_strdup(name);

	free(name);

	return result;
}

/*
 * Intended call sequence of internal iterator helper functions:
 *
 *   iter_init
 *
 *   iter_reset
 *   while (iter_advance)
 *     ...
 *
 *   iter_reset
 *   while (iter_advance)
 *     ...
 *
 *   iter_exit
 */

/* Initialize @iter. */
static void iter_init(struct yaml_iter *iter, struct yaml_node **root,
		      struct yaml_node *parent)
{
	memset(iter, 0, sizeof(*iter));
	iter->root = root ? *root : NULL;
	iter->parent = parent;
}

/* Perform cleanup of @iter. */
static void iter_exit(struct yaml_iter *iter, struct yaml_node **root)
{
	free(iter->path);

	/* Root node may have changed. */
	if (root)
		*root = iter->root;
}

/* Update @iter to point to the next YAML node. Return %true on success,
 * %false if iterator was already at end. */
static bool iter_advance(struct yaml_iter *iter, const char *parent_path)
{
	/* Advance iter->prev only if node was not removed. */
	if (iter->node)
		iter->prev = iter->node;

	iter->node = iter->next;

	free(iter->path);
	if (iter->node) {
		iter->next = iter->node->next;
		iter->path = node_path(iter->node, parent_path);
	} else {
		/* Reached end. */
		iter->next = NULL;
		iter->path = NULL;
	}

	return !!iter->node;
}

/* Reset @iter so that it will point to the first node after the next
 * iter_advance() call. */
static void iter_reset(struct yaml_iter *iter)
{
	iter->prev = NULL;
	iter->node = NULL;
	if (iter->parent)
		iter->next = get_child(iter->parent);
	else
		iter->next = iter->root;
}

static bool _traverse(struct yaml_node **root, struct yaml_node *parent,
		      const char *parent_path, yaml_cb_t cb, void *data)
{
	struct yaml_iter iter;
	bool result = true;

	iter_init(&iter, root, parent);

	iter_reset(&iter);
	while (result && iter_advance(&iter, parent_path)) {
		/* Handle node. */
		result = cb(&iter, data);
		if (!result)
			break;

		/* Handle child nodes. */
		result = _traverse(iter.node ? &iter.root : NULL, iter.node,
				   iter.path, cb, data);
	}

	iter_exit(&iter, root);

	return result;
}

/**
 * yaml_traverse - Traverse all nodes in YAML document depth-first
 * @root: Pointer to pointer to YAML document to traverse
 * @cb:   Callback to call for each node in the document
 * @data: Extra data to pass to callback
 *
 * Call @cb for all nodes in @root, or until @cb returns %false.
 *
 * Callback functions may replace or delete the current node in @root using the
 * yaml_replace() and yaml_del() functions. If the root node is modified,
 * the @root pointer will be updated accordingly.
 *
 * Return %true if all nodes were traversed, or %false otherwise.
 */
bool yaml_traverse(struct yaml_node **root, yaml_cb_t cb, void *data)
{
	return _traverse(root, NULL, "", cb, data);
}

/**
 * @a_parent: Parent node in the first document, or %NULL.
 * @b_parent: Parent node in the second document, or %NULL.
 * @a_root: Root node of the first document, or %NULL.
 * @b_root: Root node of the second document, or %NULL.
 */
static bool _traverse2(struct yaml_node **a_root, struct yaml_node *a_parent,
		       struct yaml_node **b_root, struct yaml_node *b_parent,
		       const char *parent_path, yaml_cb2_t cb, void *data)
{
	struct yaml_iter a_iter, b_iter;
	bool result = true;

	iter_init(&a_iter, a_root, a_parent);
	iter_init(&b_iter, b_root, b_parent);

	/* First pass: nodes in a and a+b. */
	iter_reset(&a_iter);
	while (result && iter_advance(&a_iter, parent_path)) {
		/* Find matching node in b. */
		iter_reset(&b_iter);
		while (iter_advance(&b_iter, parent_path)) {
			if (strcmp(a_iter.path, b_iter.path) == 0)
				break;
		}

		/* Handle nodes. */
		result = cb(&a_iter, b_iter.node ? &b_iter : NULL, data);
		if (!result)
			break;

		/* Skip calls for two NULL nodes. */
		if (!a_iter.node && !b_iter.node)
			continue;

		/* Handle child nodes. */
		result = _traverse2(a_iter.node ? &a_iter.root : NULL,
				    a_iter.node,
				    b_iter.node ? &b_iter.root : NULL,
				    b_iter.node, a_iter.path, cb, data);
	}
	if (!result)
		goto out;

	/* Second pass: nodes in b only. */
	iter_reset(&b_iter);
	while (result && iter_advance(&b_iter, parent_path)) {
		/* Find matching node in a. */
		iter_reset(&a_iter);
		while (iter_advance(&a_iter, parent_path)) {
			if (strcmp(a_iter.path, b_iter.path) == 0)
				break;
		}

		/* Skip nodes in both documents as they were already handled. */
		if (a_iter.node)
			continue;

		/* Handle nodes. */
		result = cb(NULL, &b_iter, data);
		if (!result)
			break;

		/* Skip if b was deleted. */
		if (!b_iter.node)
			continue;

		/* Handle child nodes. */
		result = _traverse2(NULL, NULL, &b_iter.root, b_iter.node,
				    b_iter.path, cb, data);
	}

out:
	iter_exit(&a_iter, a_root);
	iter_exit(&b_iter, b_root);

	return result;
}

/**
 * yaml_traverse2 - Depth-first, side-by-side traversal of 2 YAML documents
 * @a: Pointer to pointer to first YAML document to traverse
 * @b: Pointer to pointer to second YAML document to traverse
 * @cb: Callback to call for each node in the document
 * @data: Extra data to pass to callback
 *
 * Call @cb for all nodes in @a and @b, or until @cb returns %false. Call
 * @cb only once for nodes with the same YAML path in both @a and @b.
 *
 * Callback functions may replace or delete the current node in @a and @b using
 * the yaml_replace() and yaml_del() functions. If the root nodes of a and b
 * are modified, @a and @b will be updated accordingly.
 *
 * Return %true if all nodes were traversed, or %false otherwise.
 */
bool yaml_traverse2(struct yaml_node **a, struct yaml_node **b, yaml_cb2_t cb,
		    void *data)
{
	return _traverse2(a, NULL, b, NULL, "", cb, data);
}

/**
 * yaml_iter_replace - Replace node in YAML document
 * @iter: Represents node to replace
 * @replacement: Replacement node
 *
 * Insert @replacement in a YAML document in place of node identified by @iter
 * and free the previous node. If @replacement is %NULL, remove the specified
 * node.
 */
void yaml_iter_replace(struct yaml_iter *iter, struct yaml_node *replacement)
{
	struct yaml_node *t = iter->node;

	/* Replace node in iterator. */
	iter->node = replacement;

	if (replacement) {
		/* Use yaml_append here to cover multi-node replacement. */
		yaml_append(replacement, t->next);
	} else {
		/* Delete by replacing with successor. */
		replacement = t->next;
	}

	/* Replace node in YAML document. */
	if (iter->prev)
		iter->prev->next = replacement;
	else if (iter->parent)
		set_child(iter->parent, replacement);
	else
		iter->root = replacement;

	/* Free node. */
	t->next = NULL;
	yaml_free(t);
}

/**
 * yaml_iter_del - Remove node from YAML document
 * @iter: Represents node to remove
 */
void yaml_iter_del(struct yaml_iter *iter)
{
	yaml_iter_replace(iter, NULL);
}

/**
 * yaml_canon_path - Remove relative path components from YAML path
 * @path: Path to process
 *
 * Return a newly allocated string that represents YAML path @path without
 * "/../" components.
 */
char *yaml_canon_path(const char *path)
{
	char *result = misc_strdup(path), *d, *before, *after;

	d = result;
	while ((d = strstr(d, ".."))) {
		before = (d == result) ? NULL : d - 1;
		after = d + 2;

		if ((before && *before != '/') || (*after && *after != '/')) {
			/* ".." is part of component name - ignore. */
			d = after;
			continue;
		}

		if (!before) {
			/* ".." at beginning - just remove. */
			if (*after && after[1])
				after++;
			memmove(result, after, strlen(after) + 1);
			d = result;
			continue;
		}

		/*
		 * ".." between components - find start of previous component.
		 */
		*before = 0;
		before = strrchr(result, '/');
		if (!before)
			before = result;

		/* Overwrite previous component. */
		memmove(before, after, strlen(after) + 1);
		d = result;
	}

	/* Remove leading '/'. */
	if (*result == '/' && result[1])
		memmove(result, result + 1, strlen(result));

	debug("yaml_canon_path(%s)=%s", path, result);

	return result;
}

static bool free_data_cb(struct yaml_iter *iter, void *data)
{
	release_fn_t release_fn = (release_fn_t) data;

	if (release_fn)
		release_fn(iter->node->data);
	iter->node->data = NULL;

	return true;
}

/**
 * yaml_free_data - Release or clear node extra data in YAML document
 * @root: Root node of YAML document
 * @release_fn: Function for releasing extra data or %NULL
 *
 * If specified, call @release_fn for all @data fields in YAML document
 * specified by @root. Set the @data field to %NULL afterwards.
 */
void yaml_free_data(struct yaml_node *root, release_fn_t release_fn)
{
	yaml_traverse(&root, free_data_cb, release_fn);
}

static bool is_subset_cb(struct yaml_iter *a, struct yaml_iter *b, void *data)
{
	return !!b;
}

/**
 * yaml_is_subset - Check if a YAML document is a subset of another document
 * @a: First YAML document
 * @b: Second YAML document
 *
 * Return %true if all nodes in @a have a counterpart with the same YAML
 * path in @b, %false otherwise.
 */
bool yaml_is_subset(struct yaml_node *a, struct yaml_node *b)
{
	return yaml_traverse2(&a, &b, &is_subset_cb, NULL);
}

static bool cmp_cb(struct yaml_iter *a, struct yaml_iter *b, void *data)
{
	char *as, *bs;

	if (!a || !b)
		return false;

	if (a->node->type != b->node->type)
		return false;

	if (a->node->type == yaml_scalar) {
		as = a->node->scalar.content;
		bs = b->node->scalar.content;

		if (!as && !bs)
			return true;
		if (as && bs && strcmp(as, bs) == 0)
			return true;

		return false;
	}

	return true;
}

/**
 * yaml_cmp - Compare the contents of two YAML documents
 * @a: First YAML document
 * @b: Second YAML document
 *
 * Return %true if all nodes, including child nodes, in @a are present in @b,
 * %false otherwise.
 */
bool yaml_cmp(struct yaml_node *a, struct yaml_node *b)
{
	return yaml_traverse2(&a, &b, &cmp_cb, NULL);
}

/**
 * yaml_quote - Return a pointer to a copy of src in which special characters
 * are quoted.
 * The returned string must be freed by the caller
 */
char *yaml_quote(const char *src)
{
	struct misc_map map[]  = {
		{ "\\", "\\\\" },
		{ "\"", "\\\"" },
		{ "\n", "\\\\n" },
		{ NULL, NULL }
	};

	return misc_replace_map(src, map);
}

void yaml_set_handled(struct yaml_node *node)
{
	set_handled(node, true);
}

/**
 * yaml_sanitize_scalar - Print file data as valid YAML block scalar
 *
 * @in: Stream providing input data
 * @out: Target stream for resulting block scalar data
 * @indent: Number of spaces to indent
 * @convert: Flag to convert non-ASCII characters to hex notation
 *
 * Convert text data read from @in to a valid YAML block scalar indented
 * by @indent spaces. Write the resulting data to @out. If @convert is
 * %true any non-printable ASCII character will be converted to hexadecimal
 * notation ('\x<hexvalue>').
 *
 * Note: If the text read from @in is not newline-terminated a terminating
 *       newline is generated
 */
void yaml_sanitize_scalar(FILE *in, FILE *out, int indent, bool escape)
{
	char *line = NULL;
	size_t n = 0;
	ssize_t i, r;
	bool need_nl = false;

	while ((r = getline(&line, &n, in)) != -1) {
		fprintf(out, "%*s", indent, "");

		for (i = 0; i < r; i++) {
			if (!escape || isprint(line[i]) || line[i] == '\n')
				fputc(line[i], out);
			else {
				fprintf(out, "\\x%02x",
					(unsigned char) line[i]);
			}
		}
		need_nl = line[r - 1] != '\n';
	}
	free(line);

	if (need_nl)
		fprintf(out, "\n");
}
