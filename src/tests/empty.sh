#!/bin/bash
#
# Check if framework aborts on testcase directory with no tests.
#

# This is needed to keep framework from interpreting output as TAP
echo "Output for make on empty testcase directory:"

# Need to unset this or empty/Makefile would not check count
export _TELA_RUNNING=

if make -C empty/ check PRETTY=0 LOG= V=1 ; then
	echo "RC=$?"
	echo "Empty testcase directory did not abort"
	exit 1
fi

exit 0
