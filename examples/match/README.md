Tela API examples
=================

This directory contains sample testcases that demonstrate tela's APIs:

 1. Sample Makefile listing testcases and build recipes for compiled tests
 2. Simple testcases written in both Bash and C
 3. Testcases using tela's Bash and C APIs

Use `make check` to run all testcases. Detailed test output in TAP13 format
can be found in the resulting file `test.log`.

# Files

- `Makefile`

  Main Makefile defining testcase list and build recipes for compiled tests

- `dev_null.c`

  Simple testcase written in C.

- `day.c`
  `day.yaml`

  Testcase that reports multiple test results using the tela C API.

- `monday.sh`

  Simple testcase written in Bash.

- `time.sh`
  `time.sh.yaml`

  Testcase that reports multiple test results using the tela Bash API.
