#include <err.h>
#include <fnmatch.h>
#include <stdio.h>
#include <string.h>

#include "misc.h"
#include "yaml.h"

#define STR(x)	(x) ? : "<null>"
#define N(x)	(x) ? "non-null" : "null"

static const char *scalar(struct yaml_node *node)
{
	if (!node)
		return "<null>";

	switch (node->type) {
	case yaml_scalar:
		return STR(node->scalar.content);
	case yaml_seq:
		return scalar(node->seq.content);
	case yaml_map:
		return scalar(node->map.key);
	}

	return "";
}

static int count_depth(const char *path)
{
	int i, count;

	count = 0;
	for (i = 0; i < (int) strlen(path); i++) {
		if (path[i] == '/')
			count++;
	}

	return count;
}

static void handle_iter(struct yaml_iter *iter, const char *del,
			const char *rep)
{
	struct yaml_node *node = iter->node, *c;

	switch (node->type) {
	case yaml_scalar:
		printf("%s\n", STR(node->scalar.content));
		break;
	case yaml_seq:
		printf("-\n");
		break;
	case yaml_map:
		c = node->map.key;
		if (c->type == yaml_scalar)
			printf("%s:\n", STR(c->scalar.content));
		else
			printf("<nonscalar>\n");
		break;
	}

	if (fnmatch(del, scalar(iter->node), 0) == 0) {
		printf("*** Deleting node\n");
		yaml_iter_del(iter);
	} else if (fnmatch(rep, scalar(iter->node), 0) == 0) {
		printf("*** Replacing node\n");
		yaml_iter_replace(iter, yaml_parse_string("", "replacement"));
	}
}

static bool traverse2_cb(struct yaml_iter *a_iter, struct yaml_iter *b_iter,
			 void *data)
{
	int indent = count_depth(a_iter ? a_iter->path : b_iter->path);

	printf("a: ");
	if (a_iter) {
		printf("%-40s: ", a_iter->path);
		printf("%*s", indent * 2, "");

		handle_iter(a_iter, "delete_[ac]", "replace_[ac]");
	} else
		printf("<null>\n");

	printf("b: ");
	if (b_iter) {
		printf("%-40s: ", b_iter->path);
		printf("%*s", indent * 2, "");

		handle_iter(b_iter, "delete_[bc]", "replace_[bc]");
	} else
		printf("<null>\n");

	return true;
}

static int do_traverse2(const char *filea, const char *fileb)
{
	struct yaml_node *a, *b;

	a = yaml_parse_file("%s", filea);
	b = yaml_parse_file("%s", fileb);

	printf("Before (a=%s, b=%s):\n", N(a), N(b));
	printf("= a ===============================\n");
	yaml_write_stream(a, stdout, 2, false);
	printf("= b ===============================\n");
	yaml_write_stream(b, stdout, 2, false);
	printf("==================================\n\n");

	printf("Callback:\n");
	printf("==================================\n");
	yaml_traverse2(&a, &b, traverse2_cb, NULL);
	printf("==================================\n\n");

	printf("After (a=%s, b=%s):\n", N(a), N(b));
	printf("= a ==============================\n");
	yaml_write_stream(a, stdout, 2, false);
	printf("= b ==============================\n");
	yaml_write_stream(b, stdout, 2, false);
	printf("==================================\n");

	yaml_free(a);
	yaml_free(b);

	return 0;
}

static bool traverse_cb(struct yaml_iter *iter, void *data)
{
	int indent = count_depth(iter->path);

	printf("%-40s: ", iter->path);
	printf("%*s", indent * 2, "");

	handle_iter(iter, "delete", "replace");

	return true;
}

static int do_traverse(const char *filename)
{
	struct yaml_node *root;

	root = yaml_parse_file("%s", filename);

	printf("Before (root=%s):\n", N(root));
	printf("==================================\n");
	yaml_write_stream(root, stdout, 2, false);
	printf("==================================\n\n");

	printf("Callback:\n");
	printf("==================================\n");
	yaml_traverse(&root, traverse_cb, NULL);
	printf("==================================\n\n");

	printf("After (root=%s):\n", N(root));
	printf("==================================\n");
	yaml_write_stream(root, stdout, 2, false);
	printf("==================================\n");

	yaml_free(root);

	return 0;
}

int main(int argc, char *argv[])
{
	if (argc == 3 && strcmp(argv[1], "traverse") == 0)
		return do_traverse(argv[2]);
	if (argc == 4 && strcmp(argv[1], "traverse2") == 0)
		return do_traverse2(argv[2], argv[3]);

	fprintf(stderr, "Usage: %s traverse <filename>\n", argv[0]);
	fprintf(stderr, "       %s traverse2 <filename_a> <filename_b>\n",
		argv[0]);

	return 1;
}
