#!/bin/bash
#
# This is a helper tool for implementing run-cmd tests. It echoes command line
# arguments, one per line.
#

for ARG in "$@" ; do
	printf "%b\\n" "$ARG"
done

exit 0
