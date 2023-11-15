/* SPDX-License-Identifier: MIT */
/*
 * Minimal YAML-subset parser.
 *
 * Copyright IBM Corp. 2023
 *
 */

#ifndef YAML_H
#define YAML_H

#include <stdbool.h>
#include <stdio.h>

/* Replacement character for slashes in YAML path components. */
#define YAML_PATH_SLASH	'\xff'

/* Iterate @node over all entries in @root. */
#define yaml_for_each(node, root) \
	for ((node) = (root); (node); (node) = (node)->next)

/* Iterate @node over all entries in @root. Tolerate removal of @node. */
#define yaml_for_each_safe(node, root, next) \
	for ((node) = (root), (next) = (node) ? (node)->next : NULL; \
	     (node); (node) = (next), (next) = (node) ? (node)->next : NULL)


enum yaml_type {
	yaml_scalar,
	yaml_seq,
	yaml_map,
};

struct yaml_node;

struct yaml_scalar_data {
	char *content;
};

struct yaml_seq_data {
	struct yaml_node *content;
};

struct yaml_map_data {
	struct yaml_node *key;
	struct yaml_node *value;
};

struct yaml_node {
	char *filename;
	int lineno;
	bool handled;
	void *data;
	enum yaml_type type;
	union {
		struct yaml_scalar_data scalar;
		struct yaml_seq_data seq;
		struct yaml_map_data map;
	};
	struct yaml_node *next;
};

/**
 * struct yaml_iter - Per-node data passed to callback during traversal
 * @node: Current node
 * @prev: Previous node or %NULL
 * @prev: Next node or %NULL
 * @parent: Parent node or %NULL
 * @root: Root node
 * @path: Textual path to current node
 */
struct yaml_iter {
	struct yaml_node *node;
	struct yaml_node *prev;
	struct yaml_node *next;
	struct yaml_node *parent;
	struct yaml_node *root;
	char *path;
};

/**
 * yaml_cb_t - Traversal callback function
 * @iter: Node data for the current node
 * @data: Extra data as passed to yaml_traverse()
 *
 * Return %true if processing should continue, %false to abort further
 * processing.
 *
 * Note: When deleting or replacing the node identified by @iter, this callback
 * must ensure that all pointers in @iter are updated to match the new
 * situation.
 */
typedef bool (*yaml_cb_t)(struct yaml_iter *iter, void *data);

/**
 * yaml_cb2_t - Traversal callback function for dual traversal
 * @a: Node data for the current node in the first YAML document or %null if
 *     the node only exists in @b
 * @b: Node data for the current node in the second YAML document or %null if
 *     the node only exists in @a
 * @data: Extra data as passed to yaml_traverse()
 *
 * Return %true if processing should continue, %false to abort further
 * processing. Note that either @a or @b may be %NULL if a node with the same
 * path is only available in one document.
 *
 * Note: When deleting or replacing the node identified by @iter, this callback
 * must ensure that all pointers in @iter are updated to match the new
 * situation.
 */
typedef bool (*yaml_cb2_t)(struct yaml_iter *a, struct yaml_iter *b,
			   void *data);

typedef void (*release_fn_t)(void *);

struct yaml_node *yaml_parse_file(const char *fmt, ...);
struct yaml_node *yaml_parse_string(const char *name, const char *fmt, ...);
struct yaml_node *yaml_parse_stream(FILE *fd, const char *name);
void yaml_free(struct yaml_node *node);
void yaml_print(struct yaml_node *node, int indent);
struct yaml_node *yaml_get_node(struct yaml_node *root, const char *path);
char *yaml_get_scalar(struct yaml_node *root, const char *path);
void yaml_check_unhandled(struct yaml_node *root);
struct yaml_node *yaml_dup(struct yaml_node *node, bool single, bool no_child);
struct yaml_node *yaml_append(struct yaml_node *root, struct yaml_node *node);
void yaml_append_child(struct yaml_node *parent, struct yaml_node *node);
void yaml_write_stream(struct yaml_node *root, FILE *file, int indent,
		       bool single);
bool yaml_write_file(struct yaml_node *root, int indent, bool single,
		     const char *fmt, ...);
bool yaml_traverse(struct yaml_node **root, yaml_cb_t cb, void *data);
bool yaml_traverse2(struct yaml_node **a, struct yaml_node **b, yaml_cb2_t cb,
		    void *data);
void yaml_iter_replace(struct yaml_iter *iter, struct yaml_node *replacement);
void yaml_iter_del(struct yaml_iter *iter);
char *yaml_canon_path(const char *path);
void yaml_free_data(struct yaml_node *root, release_fn_t release_fn);
void yaml_decode_path(char *path);
bool yaml_is_subset(struct yaml_node *a, struct yaml_node *b);
bool yaml_cmp(struct yaml_node *a, struct yaml_node *b);
char *yaml_quote(const char * src);
void yaml_set_handled(struct yaml_node *node);
void yaml_sanitize_scalar(FILE *in, FILE *out, int indent, bool escape);

#endif /* YAML_H */
