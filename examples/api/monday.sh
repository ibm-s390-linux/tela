#!/bin/bash
#
# Check if today is a monday.
#

WEEKDAY=$(date +%w)

echo "Current weekday: $WEEKDAY"

if [ $WEEKDAY -ne 1 ] ; then
	echo "Failure: Today is not a Monday" >&2
	exit 1
fi

echo "Success: It's a Monday!"

exit 0
