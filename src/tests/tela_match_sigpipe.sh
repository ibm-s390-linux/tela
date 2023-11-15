#!/bin/bash
#
# Check if tela's destructor is called twice when there is a race between
# SIGPIPE handling and normal ending.
#

# Suppress abort on double free
export MALLOC_CHECK_=1

# Output TELA debug messages
export TELA_DEBUG=1

LOG="$TELA_TMP/log"

# Generate SIGPIPE by closing stdout while tela is running
$TELA_TOOL match /dev/null 2>$LOG | :

NUM=$(grep "running destructor" $LOG | wc -l)

echo "Output log:"
sed -e 's/^/  /' $LOG

if [[ $NUM -ne 1 ]] ; then
	echo "Failure: destructor called multiple times:"
	exit 1
fi

echo "Success: destructur called 1 time"

exit 0
