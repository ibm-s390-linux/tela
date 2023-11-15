#!/bin/bash
#
# Check for correct testexec when sourceing $TELA_BASH from an interpreter
# program.
#

D=subtests
OUT=$TELA_TMP/out
RC=0

cd $D
$TELA_TOOL run ./script.sh >$OUT

echo "Output:"
cat $OUT

TESTEXEC=$(grep testexec: $OUT)

echo "Testexec: $TESTEXEC"

if ! echo $TESTEXEC | grep '/script\.sh"$' ; then
	echo "Error: testexec not as expected" >&2
	RC=1
fi

exit $RC
