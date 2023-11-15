#!/bin/bash
#
# Check if reading from stdin blocks.
#

TIMEOUT=2

read -t $TIMEOUT

if [ $? -gt 128 ] ; then
	echo "Error: Reading from stdin blocked for more than $TIMEOUT seconds" >&2
	exit 1
fi

exit 0
