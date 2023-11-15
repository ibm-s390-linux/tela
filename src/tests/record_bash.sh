#!/bin/bash
#
# Test Bash implementation of tela record functionality.
#

TMPOUT="$TELA_TMP/out"

function fail() {
	echo "Error: $*" >&2
	exit 1
}

# Prepare environment for tela subtests
source ./clear_tela_vars.sh

make -C subtests check TESTS=record_bash.sh PRETTY=0 >"$TMPOUT" ||
	fail "Make failed"

echo "Output:"
sed -e 's/^/  /' "$TMPOUT"

grep -q "stdout:.*MARKER1" "$TMPOUT" ||
	fail "Did not find stdout marker"

grep -q "stderr:.*MARKER2" "$TMPOUT" ||
	fail "Did not find stderr marker"

! grep -q "stdout:.*MARKER3" "$TMPOUT" ||
	fail "Record stop did not work"

# Pass

exit 0
