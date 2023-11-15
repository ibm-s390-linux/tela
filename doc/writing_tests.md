Writing tests
=============

This document contains information about how to write tests using the tela
framework. See also the files in the 'examples/' subdirectory for
example test implementations.


Creating a simple test
----------------------

A _simple test_ is an executable program (e.g. a shell script or a C program)
that implements one testcase. It has the following characteristics:

  * Implements exactly one testcase per executable
  * Reports result via exit code (0=pass, 1=fail, 2=skip, 3=todo)
  * For skip and todo results, the last line written to stderr is the reason
  * The testcase name is defined by the executable name
  * All output is recorded and stored as test result

To add a _simple test_ to the tela framework, add its name to the TESTS
variable in the Makefile of the containing directory. Compiled tests
also require a recipe for building the test executable (default recipes
exist for C files).

See file 'examples/monday.sh' for an example of a simple test written in Bash.


Creating a TAP test
-------------------

A _TAP test_ is an executable program that produces output in the [TAP13
format][1]. It has the following characteristics:

  * Can implement one or more testcases per executable
  * A _plan_ must be provided in a YAML file if more than one testcase is
    implemented (see 'YAML file').
  * The program exit codes is not evaluated (but should reflect overall results)
  * The testcase name is a combination of executable name and TAP output
  * All output must be in TAP format. In particular the first line of output
    must be 'TAP version 13'

The tela framework provides a number of helper functions that can be
used to easily generate TAP output (see 'Helper functions' below).

To add a _TAP test_ to the tela framework, add its name to the TESTS
variable in the Makefile of the containing subdirectory. Compiled tests
also require a recipe for building the test executable (default recipes
exist for C files).

See file 'examples/day.c' for an example of a TAP test written in C.

[1]: https://testanything.org/tap-version-13-specification.html


Testcase result
---------------

Testcases can report one of the following results:

 * _pass:_ The testcase completed successfully
 * _fail:_ The testcase did not complete successfully
 * _skip:_ The testcase was skipped, for example because it is not applicable to
   the environment it was run in
 * _todo:_ The testcase is not fully implemented yet

While not an actual testcase result, the following can also be the outcome of
a test executable:

 * _bail out_: A fatal error occurred, all further testing (also by other other
   test executables) should be stopped. This result should only be reported
   under rare failure conditions, such as when the test environment
   can no longer be trusted to work correctly.


Makefile
--------

The Makefile in a test directory controls build and execution of
test executables. It consists of the following components:

 * _include-directive:_ This should be a directive that includes the
   tela Makefile either directly (for example using a relative path
   like '../tela.mak' or indirectly via another, project-specific
   include file like 'common.mak')
 * _TESTS-Variable:_ This variable defines the name and order of test
   executables to build and run. It can also contain the name of
   subdirectories with additional tests and associated Makefiles.
 * _TEST_TARGETS-Variable:_ This variable defines a list of Makefile targets
   that need to be built before testing can start. Typically this variable
   should contain the absolute path to programs under test.
 * A 'clean' target to remove generated files.
 * _Recipes for building executables:_ These recipes tell the tela framework
   how to build the compiled test executables. Standard rules are available
   for building executables from C source files.

Example Makefile:

```Makefile
include ../tela.mak

TESTS := test1.sh test2 tap_test3

# Recipes for building test executables
test2: test2.c
tap_test3: tap_test3.c $(TELA_OBJ)

clean:
	rm -f test2 tap_test3
```


Guidelines for writing tests
----------------------------

 * Testcase names should be concise to not overflow the width of formatted
   output, especially when using subdirectories
 * Testcase names may only consist of letters, digits, dots, minus, and the
   underscore sign
 * Add a comment to the start of each testcase source file describing what
   the test does
 * Test programs should not report multiple results for the same testcase name
