tela
====

tela is a **framework for running tests**. Test execution is controlled
by a Makefile. Test output is shown in human-readable format, or in
[TAP13 format][1]. Test programs can be written in any language, but
additional support APIs are only available for Bash and C programs.

tela supports **dynamic resource matching**: test authors can specify
[resource requirements](doc/resources.md) (e.g. number of CPUs, a certain
type of PCI device, or a remote system) that are dynamically matched
against a list of available resources. This removes the need to hard-code
IDs of required resources in test cases, allowing them to be run without
changes on different test systems.

[1]: https://testanything.org/tap-version-13-specification.html

## Quickstart

Run `make check` from the top-level tela directory to get a first impression
of tela output. This will compile and run some example tests. The resulting
output is written to the terminal and stored in a log file in TAP13 format:

```
$ make check
Running 13 tests
( 1/13) examples/api/dev_null ......... [pass]
( 2/13) examples/api/day:workday ...... [pass]
( 3/13) examples/api/day:weekend ...... [fail]
( 4/13) examples/api/day:sunday ....... [skip] Sunday is on the weekend
( 5/13) examples/api/day:holiday ...... [todo] Need table of holidays
( 6/13) examples/api/monday.sh ........ [fail]
( 7/13) examples/api/time.sh:am ....... [fail]
( 8/13) examples/api/time.sh:pm ....... [pass]
( 9/13) examples/match/one.sh ......... [pass]
(10/13) examples/match/big.sh ......... [pass]
(11/13) examples/match/all.sh ......... [pass]
(12/13) examples/match/toomany.sh ..... [skip] Missing dummy d
(13/13) examples/match/toobig.sh ...... [skip] Missing dummy a/size: >1000000
13 tests executed, 6 passed, 4 failed, 3 skipped
Result log stored in /path/to/tela/test.log
```

See the output of `make help` for a list of run-time options for running tela.

## Additional information:

 * [Writing a simple test](HOWTO.md): A quick guide
 * [API examples](examples/api/README.md): Example usage of tela API
 * [Resource matching examples](examples/match/README.md): Resource matching
   examples
 * [CONTRIBUTING](CONTRIBUTING.md): Contribution guidelines
 * [LICENSE](LICENSE): The MIT license that applies to this package
