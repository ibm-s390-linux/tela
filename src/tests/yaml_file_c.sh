#!/bin/bash
#
# Check if the yaml_file API function creates correct output.
#

source yaml_out_fn.sh

TMPOUT="$TELA_TMP/out"
RC=0

./clear_make.sh -C subtests check TESTS=yaml_file PRETTY=0 >"$TMPOUT" ||
	fail "Make failed"

# Check C API output for function yaml_file()
check_output "$TMPOUT" "yaml_file_expected"
if [ $? -ne 0 ] ; then
	echo "Error: Invalid sequence of output lines" >&2
	RC=1
fi

exit $RC
