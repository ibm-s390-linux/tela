#!/bin/bash
#
# Check if the atresult callback function works correctly
#

D=subtests

# Need this line to prevent tela from interpreting null_fmt TAP output
# as test result
echo "Output"

make -C $D atresult && $D/atresult
RC=$?
echo "RC=$RC"

if [ $RC -ne 0 ] ; then
	exit 1
fi

$D/atresult.sh
RC=$?

if [ $RC -ne 0 ] ; then
	exit 1
fi

exit 0
