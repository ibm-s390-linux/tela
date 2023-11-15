/* SPDX-License-Identifier: MIT */
/*
 * C-implementation of tela test-case API.
 *
 * Copyright IBM Corp. 2023
 */

#ifndef TAP_H
#define TAP_H

#include <stdbool.h>
#include <stddef.h>

/* Exit codes for test executables. */
#define EXIT_OK		0	/* All tests passed. */
#define	EXIT_FAIL	1	/* At least one test failed. */
#define	EXIT_SKIP	2	/* All tests were skipped. */
#define EXIT_TODO	3	/* All tests are incomplete. */
#define	EXIT_BAIL	4	/* Stop testing. */
#define EXIT_INTERNAL	255	/* Internal error. */

#define TELA_STRINGIFY_1(s)	#s
#define TELA_STRINGIFY(s)	TELA_STRINGIFY_1(s)

/**
 * pass - Report unconditional testcase success
 *
 * @name: Testcase name
 */
#define pass(name)	_pass(__FILE__, __LINE__, (name))

/**
 * fail - Report unconditional testcase failure
 *
 * @name: Testcase name
 * @reason: Optional description why test was failed
 */
#define fail(name, ...) _fail(__FILE__, __LINE__, name, \
			      ## __VA_ARGS__ , NULL)

/**
 * skip - Report that a testcase was skipped
 *
 * @name: Testcase name
 * @reason: Description why test was skipped
 */
#define skip(name, reason, ...)	_skip(__FILE__, __LINE__, (name), (reason), \
				      ## __VA_ARGS__)

/**
 * todo - Report that a testcase is not yet implemented
 *
 * @name: Testcase name
 * @reason: Description what is missing
 */
#define todo(name, reason, ...)	_todo(__FILE__, __LINE__, (name), (reason), \
				      ## __VA_ARGS__)

/**
 * ok - Report testcase result
 *
 * @cond: 0 for success, non-zero for failure
 * @name: Testcase name
 */
#define ok(cond, name)	_ok(__FILE__, __LINE__, (cond), (name), \
			    TELA_STRINGIFY(cond))

/**
 * fail_all - Report failure for all remaining planned testcases and exit
 *
 * @reason: Optional description why testcases were failed
 */
#define fail_all(...) _fail_all(__FILE__, __LINE__, ## __VA_ARGS__ , NULL)

/**
 * skip_all - Report that all remaining planned testcases were skipped and exit
 *
 * @reason: Description why testcases were skipped
 */
#define skip_all(reason, ...) _skip_all(__FILE__, __LINE__, (reason), \
					## __VA_ARGS__)

/**
 * bail - Abort test execution
 *
 * @reason: Reason for aborting execution
 */
#define bail(reason, ...)	_bail(__FILE__, __LINE__, (reason), \
				      ## __VA_ARGS__)

typedef void (*atresult_cb)(const char *name, const char *result, void *data);

/**
 * yaml - Log structured data in YAML format
 *
 * @text: Arbitrary YAML data to write to result log
 *
 * For more details see doc/functions.md
 */
void yaml(const char *text, ...);

/**
 * yaml_file - Log structured data from YAML file
 *
 * @filename: Name of file
 * @indent: Level of indentation for file data
 * @key: Optional mapping key to use
 * @escape: Optional flag specifying whether to convert non-ASCII data
 *
 * For more details see doc/functions.md
 */
void yaml_file(const char *filename, int indent, const char *key, bool escape);

/**
 * diag - Log diagnostics data
 *
 * @text: Arbitrary text to write to result log as diagnostics data
 */
void diag(const char *text, ...);

/**
 * exit_status - Retrieve combined exit status
 *
 * For more details see doc/functions.md
 */
int exit_status();

/**
 * fixname - Fix invalid characters in test name
 *
 * @testname: Original test name
 *
 * For more details see doc/functions.md
 */
void fixname(char *testname);

/**
 * log_file -  Log file as additional test result data
 *
 * @file: Path to file
 * @name: Filename in log (optional)
 *
 * For more details see doc/functions.md
 */
void log_file(char *file, char *name);

/**
 * atresult - Register callback function for tests
 *
 * @callback: Function to register
 * @data: Additional data to pass to callback (optional)
 *
 * For more details see doc/functions.md
 */
void atresult(atresult_cb cb, void *data);

/* Prototypes of internal implementations. */
void _pass(const char *file, int line, const char *name);
void _fail(const char *file, int line, const char *name, ...);
void _skip(const char *file, int line, const char *name,
	   const char *reason, ...);
void _todo(const char *file, int line, const char *name,
	   const char *reason, ...);
bool _ok(const char *file, int line, bool cond, const char *name,
	 const char *cond_str);
void _fail_all(const char *file, int line, ...);
void _skip_all(const char *file, int line, const char *reason, ...);
void _bail(const char *file, int line, const char *reason, ...);

#endif /* TAP_H */
