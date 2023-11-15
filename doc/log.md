Log file
========

When running tests, the tela framework automatically creates a log file
containing test results in [TAP13 format][1]. By default this log is written to
a file named 'test.log' in the current directory. You can change the name and
location using the 'LOG' variable when running `make`.

Depending on test type and settings, tela automatically adds a number of
structured fields in [YAML format][2] to the log for each testcase. The meaning
of these fields is described below.

Field        | Description
------------ | -----------
testresult   | Textual representation of the overall testcase result
testexec     | Path to the test executable
exitcode     | Exit code if test executable terminated normally
signal       | Signal number if test executable was killed by a signal
starttime    | The current time at the start of a testcase
stopptime    | The current time at the end of a testcase
duration\_ms | The total duration of a testcase in milliseconds
rusage       | Process resource usage during testcase (see `man getrusage`)
output       | The testcase output (see below for more information)

### Testcase output format

Testcase output is logged as lines with the following format:

```
[<timestamp>] <stream><continuation>: <text>
```

Field        | Description
-------------|--------
timestamp    | The number of seconds since the start of the testcase
stream       | Where this line was written (e.g. 'stdin' or 'stderr')
continuation | End-of-line indication: '(nonl)' = no newline


### Example log excerpt

```
not ok 6 - examples/monday.sh
  ---
    testresult: "fail"
    testexec: "/path/to/examples/monday.sh"
    exitcode: 1
    starttime: 1519379007.206932
    stoptime: 1519379007.219507
    duration_ms: 12.575
    rusage:
      utime_ms: 0.411
      stime_ms: 3.620
      maxrss_kb: 1352
      minflt: 747
      majflt: 0
      inblock: 8
      outblock: 0
      nvcsw: 17
      nivcsw: 0
    output: |
      [   0.012135] stderr: Failure: Today is not a Monday
      [   0.012135] stdout: Current weekday: 5
  ...
```

[1]: https://testanything.org/tap-version-13-specification.html
[2]: http://yaml.org
