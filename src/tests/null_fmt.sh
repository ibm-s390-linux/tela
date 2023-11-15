#!/bin/bash
#
# Check if the tela C-API can handle NULL for format strings.
#

D=subtests

# Need this line to prevent tela from interpreting null_fmt TAP output
# as test result
echo "Output"

make -C $D null_fmt && $D/null_fmt
RC=$?
echo "RC=$RC"

if [ $RC -eq 4 ] ; then
	# Bail out is accepted result
	exit 0
fi

exit 1
