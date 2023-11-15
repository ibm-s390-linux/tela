#!/bin/bash
#
# Check if the yaml API function creates correct output.
#

source yaml_out_fn.sh

TMPOUT="$TELA_TMP/out"
RC=0

./clear_make.sh -C subtests check TESTS=yaml_out_bash.sh PRETTY=0 >"$TMPOUT" ||
	fail "Make failed"

# Check Bash API output for function yaml()
check_output "$TMPOUT" "yaml_out_expected"
if [ $? -ne 0 ] ; then
	echo "Error: Invalid sequence of output lines" >&2
	RC=1
fi

exit $RC
