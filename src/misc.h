/* SPDX-License-Identifier: MIT */
/*
 * Miscellaneous helper functions.
 *
 * Copyright IBM Corp. 2023
 */

#ifndef MISC_H
#define MISC_H

#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/time.h>

#define EXIT_OK		0
#define EXIT_RUNTIME	1
#define EXIT_SYNTAX	2
#define EXIT_TESTCASE	3

#define PREAD	0
#define PWRITE	1

#define ARRAY_SIZE(x)	(sizeof(x) / sizeof((x)[0]))

#define WARN_PREFIX	"WARNING:"

#define get_varargs(fmt, str)				\
	va_list _args;					\
	char *(str);					\
	int _rc;					\
							\
	if (!fmt)					\
		str = misc_strdup("(null)");		\
	else {						\
		va_start(_args, (fmt));			\
		_rc = vasprintf(&(str), (fmt), _args);	\
		va_end(_args);				\
							\
		if (_rc == -1)				\
			oom();				\
	}

#define ind(i, fmt, ...)	printf("%*s" fmt, (i), "", ##__VA_ARGS__)
#define indf(f, i, fmt, ...)	fprintf((f), "%*s" fmt, (i), "", ##__VA_ARGS__)

#define PRTIME(f, name, tv, indent) \
	fprintf(f, "%*s" name "%ld.%06ld # %s\n", (indent), "", \
		(tv)->tv_sec, (tv)->tv_usec, _fmt_time(tv))

#define PRTIME_MS(f, name, tv, indent) \
	fprintf(f, "%*s" name "%ld.%03ld\n", (indent), "", \
		(tv)->tv_sec * 1000 + (tv)->tv_usec / 1000, \
		(tv)->tv_usec % 1000)

/**
 * misc_expand_array - Make room for one more item in array
 * @array_ptr: Pointer to array pointer
 * @num_ptr: Pointer to integer containing the array size
 */
#define misc_expand_array(array_ptr, num_ptr) \
	do { \
		*(array_ptr) = misc_realloc(*(array_ptr), \
				++(*(num_ptr)) * sizeof(*(*(array_ptr)))); \
	} while (0)

/**
 * enum tela_result_t - Enumeration of testcase results
 *
 * @TELA_PASS: Testcase finished successfully
 * @TELA_FAIL: Testcase failed
 * @TELA_SKIP: Testcase was skipped
 * @TELA_TODO: Testcase is not yet implemented
 */
enum tela_result_t {
	TELA_PASS = 0,
	TELA_FAIL = 1,
	TELA_SKIP = 2,
	TELA_TODO = 3,
};

struct stats_t {
	int passed;
	int failed;
	int skipped;
	int planned;
	int warnings;
};

struct color_t {
	const char *red;
	const char *green;
	const char *blue;
	const char *bold;
	const char *reset;
};

struct misc_map {
	const char *from;
	const char *to;
};

/* Contains codes for controlling colored output on stdout and stderr. */
extern struct color_t color, color_stderr;

/* Set if stdout is used for TAP output. */
extern bool is_stdout_tap;

/* Set if output should be verbose. */
extern bool verbose;

/* Set if debug output should be generated. */
extern int debug_level;

#define debug(fmt, ...)	while (debug_level >= 1) { \
			_debug(__FILE__, __LINE__, __func__, fmt, \
			       ##__VA_ARGS__); \
			break; \
		}
#define debug2(fmt, ...) while (debug_level >= 2) { \
			_debug(__FILE__, __LINE__, __func__, fmt, \
			       ##__VA_ARGS__); \
			break; \
		}

#define verb(fmt, ...) while (verbose) { printf(fmt, ##__VA_ARGS__); break; }

#define	misc_skip_space(s)	while (isspace(*(s))) { (s)++; }
#define	misc_skip_digit(s)	while (isdigit(*(s))) { (s)++; }

#define oom(name)      _oom(__FILE__, __LINE__)
void _oom(const char *file, int line);
void _debug(const char *file, int line, const char *fn, const char *fmt, ...);
const char *_fmt_time(struct timeval *tv);
char *misc_strdup(const char *s);
char *misc_asprintf(const char *fmt, ...);
void *misc_malloc(size_t size);
void *misc_realloc(void *ptr, size_t size);
char *misc_get_toplevel(void);
void misc_strip_space(char *s);
char *misc_escape(const char *src, const char *esc);
bool misc_starts_with(const char *str, const char *s);
bool misc_ends_with(const char *str, const char *s);
const char *misc_relpath(const char *path, const char *base);
char *misc_abspath(const char *path);
const char *misc_framework_dir(void);
void misc_swapcwd(const char *dir);
FILE *misc_internal_cmd(const char *subdir, const char *fmt, ...);
char *misc_mktempdir(const char *preferred);
FILE *misc_mktempfile(char **name_ptr);
int misc_system(const char *fmt, ...);
void twarn(const char *filename, int lineno, const char *fmt, ...);
void *misc_dirname(const char *path);
void *misc_basename(const char *path);
bool misc_exists(const char *path);
void misc_remove(const char *path);
void misc_flush_cleanup(void);
void misc_fix_testname(char *name);
char *misc_replace(const char *str, const char *from, const char *to);
char *misc_replace_map(const char *str, struct misc_map map[]);
void misc_chomp(char *s);
void misc_add_one_env(char ***env_ptr, int *num_ptr, const char *key,
		      const char *value);
bool misc_unquote(char *str, struct misc_map *single_map,
		  struct misc_map *double_map);
void misc_cloexec(int fd);

#endif /* MISC_H */
