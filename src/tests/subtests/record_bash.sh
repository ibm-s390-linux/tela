#!/bin/bash
#
# Sample test that uses the Bash implementation of tela's record function to
# record output to stdout and stderr.
#

source $TELA_BASH || exit 1

record all

# MARKER1 must be logged as coming from stdout
echo "MARKER1"
pass "test"

# MARKER2 must be logged as coming from stderr
echo "MARKER2" >&2
pass "test2"

record stop

# MARKER3 must not be logged as output
echo "# MARKER3"
pass "test3"

exit $(exit_status)
