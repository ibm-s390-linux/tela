/* SPDX-License-Identifier: MIT */
/*
 * Miscellaneous helper functions.
 *
 * Copyright IBM Corp. 2023
 */

#include <ctype.h>
#include <dirent.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <limits.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "misc.h"

struct color_t color, color_stderr;

bool is_stdout_tap = false;
bool verbose;
int debug_level = 0;
static char *_toplevel;
static bool _toplevel_done;

void _oom(const char *file, int line)
{
	errx(EXIT_RUNTIME, "%s:%d: Out of memory", file, line);
}

static struct timeval _debug_last_tv;

void _debug(const char *file, int line, const char *fn, const char *fmt, ...)
{
	struct timeval now;
	char *str;
	va_list args;
	int rc;

	gettimeofday(&now, NULL);
	timersub(&now, &_debug_last_tv, &now);
	va_start(args, fmt);
	rc = vasprintf(&str, fmt, args);
	va_end(args);

	if (rc == -1)
		oom();

	misc_strip_space(str);
	fprintf(stderr, "DEBUG: [%6ldms] %6u: %10s:%4d: %s: %s\n",
		now.tv_sec * 1000 + now.tv_usec / 1000, getpid(), file, line,
		fn, str);
	free(str);
}

static char **cleanup_tmp;
static int cleanup_tmp_num;

static void __attribute__((destructor)) misc_dtr(void)
{
	static bool done;
	int i;

	if (done)
		return;
	done = true;
	debug("running destructor");
	for (i = 0; i < cleanup_tmp_num; i++) {
		misc_remove(cleanup_tmp[i]);
		free(cleanup_tmp[i]);
	}
	free(cleanup_tmp);
	cleanup_tmp = NULL;
	cleanup_tmp_num = 0;
	free(_toplevel);
	_toplevel_done = false;

	fflush(stdout);
	fflush(stderr);
}

static void signal_handler(int signum)
{
	debug("got signal %d", signum);

	/* Ensure destructor is called. */
	misc_dtr();

	/* Call default handler. */
	signal(signum, SIG_DFL);
	raise(signum);
}

static void add_cleanup_tmp(const char *path)
{
	cleanup_tmp_num++;
	cleanup_tmp = misc_realloc(cleanup_tmp,
				   cleanup_tmp_num * sizeof(char *));
	cleanup_tmp[cleanup_tmp_num - 1] = misc_strdup(path);
}

/*
 * Remove any scheduled cleanup work.
 */
void misc_flush_cleanup(void)
{
	/* Cleanup will be done in parent. */
	free(cleanup_tmp);
	cleanup_tmp = NULL;
	cleanup_tmp_num = 0;
}

static void _init_colors(struct color_t *color, bool use_color)
{
	if (use_color) {
		color->red	= "\e[31m";
		color->green	= "\e[32m";
		color->blue	= "\e[34m";
		color->bold	= "\e[1m";
		color->reset	= "\e[0m";
	} else {
		color->red	= "";
		color->green	= "";
		color->blue	= "";
		color->bold	= "";
		color->reset	= "";
	}
}

static void __attribute__((constructor)) misc_ctr(void)
{
	unsigned long long l;
	char *v;

	v = getenv("_TELA_STARTTIME");
	if (v) {
		l = atoll(v);
		_debug_last_tv.tv_sec = l / 1000;
		_debug_last_tv.tv_usec = (l % 1000) * 1000;
	} else {
		gettimeofday(&_debug_last_tv, NULL);
		v = misc_asprintf("%lu%03lu", _debug_last_tv.tv_sec,
				  _debug_last_tv.tv_usec / 1000);
		setenv("_TELA_STARTTIME", v, 1);
		free(v);
	}
	debug("running constructor");
	v = getenv("TELA_DEBUG");
	if (v)
		debug_level = atoi(v);
	v = getenv("TELA_VERBOSE");
	if (v)
		verbose = atoi(v) > 1;
	v = getenv("COLOR");
	if (!v || strcmp(v, "auto") == 0) {
		_init_colors(&color, isatty(STDOUT_FILENO));
		_init_colors(&color_stderr, isatty(STDERR_FILENO));
	} else {
		_init_colors(&color, atoi(v));
		_init_colors(&color_stderr, atoi(v));
	}

	/* Ensure that destructor is called even when killed by signal. */
	signal(SIGHUP, signal_handler);
	signal(SIGINT, signal_handler);
	signal(SIGQUIT, signal_handler);
	signal(SIGTERM, signal_handler);
	signal(SIGPIPE, signal_handler);
}

char *misc_strdup(const char *s)
{
	char *result = strdup(s);

	if (!result)
		oom();

	return result;
}

char *misc_asprintf(const char *fmt, ...)
{
	char *str;
	va_list args;
	int rc;

	va_start(args, fmt);
	rc = vasprintf(&str, fmt, args);
	va_end(args);

	if (rc == -1)
		oom();

	return str;
}

void *misc_malloc(size_t size)
{
	void *buffer;

	buffer = malloc(size);
	if (!buffer)
		oom();
	memset(buffer, 0, size);

	return buffer;
}

void *misc_realloc(void *ptr, size_t size)
{
	void *buffer;

	buffer = realloc(ptr, size);
	if (!buffer)
		oom();

	return buffer;
}

#define TOP_MARKER	"/tela.mak"

/*
 * Return a pointer to the name of the top-level directory of this repository.
 */
char *misc_get_toplevel(void)
{
	char *start_dir, *d;
	int rc;
	struct stat buf;

	if (_toplevel_done)
		return _toplevel;

	/* Consult environment first. */
	d = getenv("TELA_BASE");
	if (d) {
		_toplevel = misc_strdup(d);
		goto out;
	}

	/* Start with tool dir or current working directory. */
	start_dir = realpath(program_invocation_name, NULL);
	if (start_dir) {
		d = strrchr(start_dir, '/');
		if (d)
			*d = 0;
	} else {
		start_dir = get_current_dir_name();
		if (!start_dir) {
			err(EXIT_RUNTIME, "Could not determine current "
			    "working directory");
		}
	}

	/* Find first parent containing marker. */
	_toplevel = misc_asprintf("%s%s", start_dir, TOP_MARKER);
	for (d = _toplevel + strlen(start_dir); d;
	     d = strrchr(_toplevel, '/')) {
		strcpy(d, TOP_MARKER);
		rc = stat(_toplevel, &buf);
		*d = 0;

		if (rc == 0)
			break;
	}
	free(start_dir);

	if (!d) {
		free(_toplevel);
		_toplevel = NULL;
	}

out:
	_toplevel_done = true;

	return _toplevel;
}

void misc_strip_space(char *s)
{
	size_t i;

	for (i = strlen(s); i > 0 && isspace(s[i - 1]); i--)
		s[i - 1] = 0;
}

/*
 * Return a pointer to a string which contains a copy of src, where
 * characters in @esc are escaped.
 * The returned string must be freed by the caller.
 */
char *misc_escape(const char *src, const char *esc)
{
	char *dst, *ptr;

	dst = ptr = misc_malloc(strlen(src) * 2 + 1);
	while (*src) {
		if (strchr(esc, *src) || *src == '\\')
			*ptr++ = '\\';
		*ptr++ = *src++;
	}
	return dst;
}

bool misc_starts_with(const char *str, const char *s)
{
	size_t len = strlen(s);

	if (strncmp(str, s, len) == 0)
		return true;

	return false;
}

bool misc_ends_with(const char *str, const char *s)
{
	size_t l1 = strlen(str), l2 = strlen(s);

	if (l1 < l2)
		return false;
	if (strcmp(str + l1 - l2, s) != 0)
		return false;

	return true;
}

const char *misc_relpath(const char *path, const char *base)
{
	const char *result = path;

	if (!base)
		base = getenv("TELA_TESTBASE");

	if (base && misc_starts_with(path, base)) {
		result += strlen(base);
		if (result[0] == '/')
			result++;
	}

	return result;
}

/* Return absolute path to @path without resolving links in basename. */
char *misc_abspath(const char *path)
{
	char *result, *dir, *absdir, *file;

	dir = misc_dirname(path);
	absdir = realpath(dir, NULL);
	free(dir);

	if (!absdir)
		return NULL;

	file = misc_basename(path);
	result = misc_asprintf("%s/%s", absdir, file);

	free(file);
	free(absdir);

	return result;
}

const char *misc_framework_dir(void)
{
	char *dir;

	dir = getenv("TELA_FRAMEWORK");
	if (!dir) {
		/* Use toplevel as fallback. */
		dir = misc_get_toplevel();
	}

	return dir;
}

void misc_swapcwd(const char *dir)
{
	static char *saved_dir;

	if (dir) {
		free(saved_dir);
		saved_dir = get_current_dir_name();
		if (!saved_dir)
			oom();
		chdir(dir);
	} else if (saved_dir) {
		chdir(saved_dir);
		free(saved_dir);
		saved_dir = NULL;
	}
}

/* Start internal command and return filehandle to output pipe or %NULL on
 * error. */
FILE *misc_internal_cmd(const char *subdir, const char *fmt, ...)
{
	const char *basedir;
	char *dir, *abscmd;
	FILE *fd;

	get_varargs(fmt, cmd);

	basedir = misc_framework_dir();
	dir = misc_asprintf("%s/src/libexec/%s", basedir, subdir);
	abscmd = misc_asprintf("%s/%s", dir, cmd);

	misc_swapcwd(dir);
	fd = popen(abscmd, "r");
	misc_swapcwd(NULL);

	free(abscmd);
	free(dir);
	free(cmd);

	return fd;
}

/* Create a temporary directory. */
char *misc_mktempdir(const char *preferred)
{
	const char *tmpdirs[] = { getenv("TMPDIR"), preferred, P_tmpdir,
				  "/tmp" };
	char *name = NULL;
	int i;

	for (i = 0; i < (int) ARRAY_SIZE(tmpdirs) && !name; i++) {
		if (!tmpdirs[i])
			continue;
		name = misc_asprintf("%s/tela.XXXXXX", tmpdirs[i]);
		if (!mkdtemp(name)) {
			free(name);
			name = NULL;
		}
	}
	if (!name)
		errx(EXIT_RUNTIME, "Could not create temporary directory");

	add_cleanup_tmp(name);

	return name;
}

/* Create a temporary file. */
FILE *misc_mktempfile(char **name_ptr)
{
	const char *tmpdirs[] = { getenv("TMPDIR"), P_tmpdir, "/tmp" };
	char *name = NULL;
	int i, fd;
	FILE *file;

	for (i = 0; i < (int) ARRAY_SIZE(tmpdirs) && !name; i++) {
		if (!tmpdirs[i])
			continue;
		name = misc_asprintf("%s/tela.XXXXXX", tmpdirs[i]);
		fd = mkstemp(name);
		if (fd == -1) {
			free(name);
			name = NULL;
		}
	}
	if (fd == -1)
		goto err;

	file = fdopen(fd, "w+");
	if (!file)
		goto err;

	add_cleanup_tmp(name);

	if (name_ptr)
		*name_ptr = name;
	else
		free(name);

	return file;

err:
	err(EXIT_RUNTIME, "Could not create temporary file");
}

/* Perform the command specified by @fmt. */
int misc_system(const char *fmt, ...)
{
	int rc;

	get_varargs(fmt, cmd);

	rc = system(cmd);
	free(cmd);

	return rc;
}

/**
 * twarn - Print and log warning messages
 *
 * @filename: Name of file associated with warning (or %NULL if not needed)
 * @lineno: Line number associated with warning (or 0 if not needed)
 * @fmt: Warning message
 */
void twarn(const char *filename, int lineno, const char *fmt, ...)
{
	FILE *fd = stderr;
	const char *prefix, *suffix;

	get_varargs(fmt, str);
	misc_strip_space(str);

	if (filename)
		filename = misc_relpath(filename, NULL);

	if (is_stdout_tap) {
		/* TAP-format diagnostics data. */
		fd = stdout;
		prefix = "# ";
		suffix = "";
	} else {
		/* Colored output on stderr. */
		fd = stderr;
		prefix = color_stderr.red;
		suffix = color_stderr.reset;
	}

	fprintf(fd, "%s%s ", prefix, WARN_PREFIX);
	if (filename && lineno > 0)
		fprintf(fd, "%s:%d: ", filename, lineno);
	else if (filename)
		fprintf(fd, "%s: ", filename);
	fprintf(fd, "%s%s\n", str, suffix);

	free(str);
}

void *misc_dirname(const char *path)
{
	char *copy, *result;

	copy = misc_strdup(path);
	result = misc_strdup(dirname(copy));
	free(copy);

	return result;
}

void *misc_basename(const char *path)
{
	char *copy, *result;

	copy = misc_strdup(path);
	result = misc_strdup(basename(copy));
	free(copy);

	return result;
}

bool misc_exists(const char *path)
{
	struct stat buf;

	return stat(path, &buf) == 0;
}

void misc_remove(const char *path)
{
	struct stat buf;
	struct dirent *de;
	DIR *dir;
	char *name;

	if (lstat(path, &buf) != 0)
		return;

	if (S_ISDIR(buf.st_mode)) {
		dir = opendir(path);
		if (!dir) {
			warnx("Could not open directory %s", path);
			return;
		}

		while ((de = readdir(dir))) {
			if (strcmp(de->d_name, ".") == 0 ||
			    strcmp(de->d_name, "..") == 0)
				continue;
			name = misc_asprintf("%s/%s", path, de->d_name);
			misc_remove(name);
			free(name);
		}

		closedir(dir);
	}

	debug("removing %s", path);
	remove(path);
}

const char *_fmt_time(struct timeval *tv)
{
	static char buffer[30];
	struct tm *tm;

	tm = localtime(&tv->tv_sec);
	if (!tm || (strftime(buffer, sizeof(buffer), "%F %T%z", tm) == 0))
		return "";

	return buffer;
}

static bool valid_testname_char(char c)
{
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
	       (c >= '0' && c <= '9') || c == '.' || c == '_' || c == '-';
}

void misc_fix_testname(char *name)
{
	int from, to;
	bool insert = false;

	/* Replace invalid characters in test name. */
	for (from = to = 0; name[from]; from++) {
		if (valid_testname_char(name[from])) {
			if (insert) {
				if (to > 0 && name[to - 1] != '_')
					name[to++] = '_';
				insert = false;
			}
			name[to++] = name[from];
		} else
			insert = true;
	}

	/* Ensure at least one character. */
	if (insert && to == 0)
		name[to++] = '_';

	name[to] = 0;
}

/* Return a newly allocated copy of @str in which all occurrences of @from are
 * replaced with @to. */
char *misc_replace(const char *str, const char *from, const char *to)
{
	size_t from_len = strlen(from), to_len = strlen(to), num = 0;
	char *c, *copy;

	if (from_len == 0)
		return misc_strdup(str);

	/* Count occurrences. */
	for (c = (char *) str; (c = strstr(c, from)); c += from_len)
		num++;

	/* Allocate slightly more space than needed to simplify processing. */
	copy = misc_malloc(strlen(str) + 1 + num * strlen(to));
	strcpy(copy, str);

	/* Replace occurrences. */
	for (c = copy; (c = strstr(c, from)); c += to_len) {
		memmove(c + to_len, c + from_len, strlen(c + from_len) + 1);
		memcpy(c, to, to_len);
	}

	return copy;
}

/* Return a newly allocated copy of @str in which all occurrences of @map->from
 * are replaced with @map->to. @map must be NULL-terminated. */
char *misc_replace_map(const char *str, struct misc_map map[])
{
	struct misc_map *m;
	char *result, *to;
	size_t newlen;

	/* Calculate maximum result length assuming each char in @str is
	 * replaced by longest target in map. */
	newlen = 1;
	for (m = map; m->from && m->to; m++) {
		if (strlen(m->to) > newlen)
			newlen = strlen(m->to);
	}
	newlen = newlen * strlen(str) + 1;

	to = result = misc_malloc(newlen);
	while (*str) {
		for (m = map; m->from && m->to; m++) {
			if (misc_starts_with(str, m->from))
				break;
		}
		if (m->to) {
			strcpy(to, m->to);
			to += strlen(m->to);
			str += strlen(m->from);
		} else {
			*to++ = *str++;
		}
	}

	return result;
}

/* Remove all trailing '\n' in @s. */
void misc_chomp(char *s)
{
	char *e = s + strlen(s) - 1;

	while (e >= s && *e == '\n')
		*(e--) = 0;
}

/* Extend list of environment variables contained in @env_ptr with specified
 * @key and @value and increase @num_ptr by one. */
void misc_add_one_env(char ***env_ptr, int *num_ptr, const char *key,
		      const char *value)
{
	char **env = *env_ptr;
	int num = *num_ptr;

	env = misc_realloc(env, sizeof(char *) * (num + 1));
	env[num - 1] = misc_asprintf("%s=%s", key, value);
	env[num] = NULL;

	*env_ptr = env;
	*num_ptr = num + 1;
}

/*
 * Unquote @str by removing surrounding quoting characters and reversing
 * escaping of characters depending on type of quotes used: apply @single_map
 * if single-quotes were found or @double_map if double-quotes were found.
 *
 * Return %true if quote removal was successful, %false if no closing quote
 * character was found.
 *
 * Note that the resulting string might be truncated if either replacement
 * mapping increases the resulting string length.
 */
bool misc_unquote(char *str, struct misc_map *single_map,
		  struct misc_map *double_map)
{
	char quote = str[0], *unescaped;
	size_t len;

	if (quote != '\'' && quote != '\"')
		return true;

	/* Remove leading quote. */
	memmove(str, str + 1, strlen(str + 1) + 1);
	len = strlen(str);

	/* Remove trailing quote. */
	if (len == 0 || quote != str[len - 1])
		return false;
	str[len - 1] = 0;

	/* Perform unescaping. */
	if (quote == '\'' && single_map)
		unescaped = misc_replace_map(str, single_map);
	else if (quote == '\"' && double_map)
		unescaped = misc_replace_map(str, double_map);

	if (unescaped) {
		strncpy(str, unescaped, strlen(str) + 1);
		free(unescaped);
	}

	return true;
}

/* Mark file descriptor @fd to be closed-on-exec. */
void misc_cloexec(int fd)
{
	if (fcntl(fd, F_SETFD, FD_CLOEXEC) != 0)
		warn("Could not set FD_CLOEXEC on fd %d", fd);
}
