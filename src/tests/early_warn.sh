#!/bin/bash
#
# Check if C-API writes TAP as first line of output even in case of early
# warnings.
#

RC=0
D=subtests

make -C $D early_warn || exit 1

OUT=$($D/early_warn 2>&1)
echo "Output: $OUT"

if [ "${OUT:0:3}" != "TAP" ] ; then
	echo "Error: Output doesn't start with 'TAP'" >&2
	RC=1
fi

exit $RC
