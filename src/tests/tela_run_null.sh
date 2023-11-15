#!/bin/bash
#
# Check if 'tela run' correctly processes commands with no output.
#

TELA="$TELA_FRAMEWORK/src/tela"
OUT="$TELA_TMP/out"
CMD=$(type -P true)

# Filter through 'tela run'
$TELA run $CMD >$OUT 2>&1

echo "Output"
cat $OUT

if ! grep -q 'output: ""' < $OUT ; then
	echo "Error: Output not as expected" >&2
	exit 1
fi

exit 0
