Helper functions
================

The tela framework provides a number of functions to help generating
TAP13-compatible output. These functions are available for test executables
written in Bash and C.

### C programs

For C programs, function definitions are available by including the tela.h
header file:

```C
#include "tela.h"
```

The executable must be linked with the object files defined by Makefile
variable $(TELA\_OBJ).

```Makefile
prog: prog.c $(TELA_OBJ)
```

### Bash programs

For Bash programs, function implementations are available by sourcing the
$TELA\_BASH file:

```Bash
source $TELA_BASH || exit 1
```

Note: Some tela Bash functions make use of the Bash EXIT handler. Test programs
must therefore not install a handler for the EXIT event via the trap command.

### Available Functions:

  - `pass(name)`

    @name: Testcase name

    The testcase with the specified @name completed successfully.

  - `fail(name, reason)`

    @name: Testcase name
    @reason: Description why test was failed (optional)

    The testcase with the specified @name failed due to @reason. @reason
    is an optional parameter.

  - `skip(name, reason)`

    @name: Testcase name
    @reason: Description why test was skipped

    The testcase with the specified @name was skipped due to @reason.

  - `todo(name, rason)`

    @name: Testcase name
    @reason: Description what is missing

    The testcase with the specified @name is not yet implemented. @reason
    describes what is missing.

  - `ok(cond, name)`

    @cond: %true for success, %false for failure
    @name: Testcase name

    The boolean condition @cond determines if the test with the specified
    @name was successful (%true) or not (%false). Return @cond.

    Note: To be consistent with programming language conventions, the Bash
    implementation of this function expects 0 for %true and non-zero for %false,
    while the C implementation expects 0 for %false and non-zero for %true.

  - `fail_all(reason)`

    @reason: Description why tests were failed (optional)

    Report all remaining planned testcases as failed due to @reason, then exit.
    @reason is an optional parameter.

  - `skip_all(reason)`

    @reason: Description why tests were skipped

    Report all remaining planned testcases as skipped due to @reason, then
    exit.

  - `bail(reason)`

    @reason: Description why further testing should be aborted

    Stop all further testing, including tests implemented by other test
    executables.

    Note: This function should only be used in extreme circumstances, such
    as when the test environment can no longer be trusted to work correctly.

  - `yaml(text)`

    @text: Arbitrary YAML data to write to result log

    Write @text to the TAP log. @text must be in YAML format.

    Note: The YAML data will be added to the YAML data block of the next
    testcase result that is reported.

  - `yaml_file(filename, indent, key, escape)`

    @filename: Name of file
    @indent: Level of indentation for file data
    @key: Optional mapping key to use
    @escape: Optional flag specifying whether to convert non-ASCII data

    Write the contents of the file specified by @filename to the TAP log. Indent
    each line with @indent spaces. File contents may be in YAML format or
    text format.

    When in YAML format @file must contain a valid YAML mapping.

    When in text format, a @key must be specified. The resulting output will
    be a YAML mapping with the specified @key and the file contents as value.
    If the file contents contains non-printable characters, @escape must
    be specified as %true.

    Note: The YAML data will be added to the YAML data block of the next
    testcase result that is reported.

    Example:
    ```C
    yaml_file("stdout.txt", 0, "stdout", true);
    ```

  - `diag(text)`

    @text: Arbitrary text to write to the result log

    Write @text to the TAP log as unstructured diagnostics data (using the
    '#' prefix).

  - `exit_status()`

    Return (C) or print (Bash) an exit code that represents the results of
    all previously reported testcase results. The result of this function
    should be used by test executables as exit code.

  - `fixname(testname)`

    @testname: Original test name

    Check the provided @testname for invalid characters. Replace any invalid
    character found with an underscore character, while suppressing duplicate
    underscores, or underscores at the start or end of the test name. Print
    the resulting test name (Bash) or return it in the original string (C).

    Note: Valid testname characters are 0-9a-zA-Z._-

  - `log_file(file, name)`

    @file: Path to file
    @name: Filename in log (optional)

    Log the contents of @file as additional test result data. The file will be
    stored either under the original name, or the optionally specified @name in a
    test-specific sub-directory in the additional test result data archive.

 - `atresult(callback, data)`

    @callback: Function to register
    @data: Additional data to pass to callback (optional)

    Register a callback that is called at the end of each test. The callback
    receives the name and result of the test and optional additional data.

 - `push_cleanup(callback, data)`

    @callback: Function to be called at end of test
    @data: Additional data to pass to callback (optional)

    Register a cleanup function that is called when the test program ends.
    When run the callback receives the optional additional data as parameter.
    Cleanup functions are called in reverse order of registration, i.e. the
    function registered last is called first.

    Note: This function is only available for test executables written in
          Bash.

 - `pop_cleanup(num)`

    @num: Number of callbacks to unregister (optional, default is 1)

    Unregister the specified number of cleanup functions without calling them.
    Functions are unregistered in reverse order, i.e. starting with the most
    recently registered cleanup function.

    Note: This function is only available for test executables written in
          Bash.

 - `record(scope)`

    @scope: Scope of output recording

    Start time-stamped recording of all subsequent output of the test
    executable to any of the specified streams specified by @scope.

    Valid values for @scope are:

    - all:    Record output to standard output and error streams
    - stdout: Record output to standard output stream only
    - stderr: Record output to standard error stream only
    - stop:   Stop any previously started recording

    Recorded output is automatically added as YAML data to test result data
    when test case results are generated using pass, fail, skip or ok.

    Note: This function is only available for test executables written in
          Bash.

 - `record_get(scope, meta)`

   @scope: Scope of returned data
   @meta: Flag specifying if timestamps and stream names should be part of log

   Return the log of output data that has been recorded since the most recent
   call to the `record` command. @scope defines the scope of data to be
   returned. Output includes timestamp and stream names if @meta is specified
   as 1, or only the raw output if @meta is 0.

   Valid values for @scope are:
    - all:    Return output to standard output and error streams
    - stdout: Return output to standard output stream only
    - stderr: Return output to standard error stream only

   Note that the recording does not need to be stopped to retrieve current
   data. Also note that reporting test results via pass, fail, skip, or ok will
   reset this log.

 - `run_cmd(cmd, name, expect_rc, expect_stdout, expect_stderr)`

   @cmd: Command line to run
   @name: Test name
   @expect_rc: Expected exit code
   @expect_stdout: Flag indicating whether output on stdout is expected
   @expect_stderr: Flag indicating whether output on stderr is expected

   Run command line @cmd and report success for test @name if the command
   exit code and output on stdout and stderr match expected values.

   @expect_rc defines the expected exit code. Non-zero @expect_stdout and
   @expect_stderr values indicate that output is expected on stdout and stderr
   respectively. If a value of 2 is provided it is ignored if there is an actual
   output on stderr/stdout or not but it is still printed.

   After this function returns, command output will be available in $TELA_TMP
   named ${@name}_stdout and ${@name}_stderr.
