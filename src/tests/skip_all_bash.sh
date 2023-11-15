#!/bin/bash
#
# Check if the 'skip_all' Bash API function reports the correct number of
# skipped testcases.
#

source all_fn.sh

D=subtests
OUT=$TELA_TMP/out
RC=0

$TELA_TOOL run $D/skip_all_bash.sh >$TELA_TMP/out

check_output $OUT "SSSSSS"
if [ $? -ne 0 ] ; then
	echo "Error: Invalid sequence of output lines" >&2
	RC=1
fi

exit $RC
