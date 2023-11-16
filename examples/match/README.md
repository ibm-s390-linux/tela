Resource matching examples
==========================

This directory contains sample resource matching configurations to showcase
how tela handles resource management for test cases.

Use `make check` to run all testcases. Detailed test output in TAP13 format
can be found in the resulting file `test.log`.

# Files

- `Makefile`

  Main Makefile defining testcase list and build recipes for compiled tests

- `resources.yaml`

  List of all resources available for the current test cases.

- `all.yaml`

  Match all available resources but make sure at least two are matched

- `all.sh`

  Output all matched resources with their attributes.

- `big.yaml`

  Try to match a resource with the desired size.

- `one.yaml`

  Try to match a single resource.

- `big.sh`
  `one.sh`

  Output matched resource(s).

- `toobig.yaml`

  Try to match a resource with impossibly big size.

- `toomany.yaml`

  Try to match more resources than available.

- `toobig.sh`
  `toomany.sh`

  Skip tests since resource matching will fail.
