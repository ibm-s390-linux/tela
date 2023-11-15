Integrating tela
================

The tela framework can be used as a standalone repository, or it can be
integrated into existing source code projects.


Standalone testcase repositories
--------------------------------

A standalone testcase repository is a repository that is not integrated in an
actual source code project. To start such a repository, simply create a copy
of the tela framework repository, create a "tests" subdirectory, and put all
testcases into the "tests" subdirectory.


Integrated testcase repositories
--------------------------------

Follow these steps to add the tela framework to an existing Makefile-based
source code project:

  1. Copy the tela repository to a subdirectory of the project
  2. Include the 'tela.mak' file from the project's Makefiles
     (ideally this is done via a common Makefile that is already included by
     all project Makefiles)
  3. Specify TELA\_NODEFRECIPES=0 before including tela.mak. This will
     instruct the tela framework not to define default recipes for 'all'
     and 'clean' Makefile targets.
  4. In the top-level Makefile, add a 'TESTS :=' statement listing all
     subdirectories that contain testcases. This enables users to run
     'make check' from the top-level directory.
  5. Consider defining and exporting variables containing the absolute path to
     the project's source and build directories. These can be used by testcases
     to easily find the files under test.


Makefile variables
------------------

The following Makefile variables are evaluated by tela.mak:

  - *`TELA_NODEFRECIPES`* (default '1')
    When set to 1, tela.mak will define recipes for common Makefile targets
    'all' and 'clean'. When set to 0, tela.mak will not provide these targets.
  - *`TELA_NUMDOTS`* (default '31')
    Specifies the number of dots to use for padding test names in the formatted
    test output. When set to -1, an alternate format for test result output
    is used.

    Example output for TELA_NUMDOTS>0:

    (1/3) test1 .............. [pass]

    Example output for TELA_NUMDOTS=-1:

    (1/3) [pass] test1
  - *`TELA_TESTSUITE`* (default is the base directory name)
    Specifies the test suite name.
