#!/bin/bash
#
# Check if the 'fail_all' C API function reports the correct number of testcase
# failures.
#

source all_fn.sh

D=subtests
OUT=$TELA_TMP/out
RC=0

make -C $D fail_all || exit 1
$TELA_TOOL run $D/fail_all >$TELA_TMP/out

check_output $OUT "FFFFFF"
if [ $? -ne 0 ] ; then
	echo "Error: Invalid sequence of output lines" >&2
	RC=1
fi

exit $RC
