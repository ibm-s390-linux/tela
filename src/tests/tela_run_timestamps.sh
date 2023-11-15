#!/bin/bash
#
# Check if tela correctly records timestamps of test program output.
#

RC=0

IFS=$'\n'
{
	read line1
	read line2
} < <($TELA_TOOL run subtests/delay.sh | grep stdout)

echo "line1: '$line1'"
echo "line2: '$line2'"

if [[ "$line1" == "$line2" ]] ; then
	echo "Error: Delay not reflected in timestamps" >&2
	RC=1
fi

exit $RC
