#!/bin/bash
#
# Test Bash implementation of tela record_get functionality.
#
# For this purpose this program:
# - starts recording
# - generates output on stdout and stderr
# - retrieves recorded data using record_get at various points in time
#   with varying parameters
# - compares recorded data with expected data
#

source $TELA_BASH || exit 1

D=$TELA_TMP

function check_file() {
	local fname="$1" need_stdout="$2" need_stderr="$3" need_meta="$4"
	local data has_stdout=0 has_stderr=0 has_meta=0

	data=$(<$fname) || skip_all "Could not read $fname"

	[[ "$data" =~ MARKER1 ]] && has_stdout=1
	[[ "$data" =~ MARKER2 ]] && has_stderr=1
	[[ "$data" =~ (stdout|stderr) ]] && [[ "$data" =~ '[' ]] && has_meta=1

	yaml "expect: stdout=$need_stdout stderr=$need_stderr meta=$need_meta"
	yaml "actual: stdout=$has_stdout stderr=$has_stderr meta=$has_meta"
	yaml "log: |"
	yaml_file "$fname" 2

	[[ "$has_stdout$has_stderr$has_meta" == \
	   "$need_stdout$need_stderr$need_meta" ]]

	ok $? "${fname##*/}"
}

# Generate log
record all

# Record must start out empty
record_get >"$D/zero"
if [[ -s "$D/zero" ]] ; then
	yaml "log: |"
	yaml_file "$D/zero" 2
	fail "zero"
else
	pass "zero"
fi

echo MARKER1
echo MARKER2 >&2

# Retrieve captured data
record_get >"$D/defaults"

record_get all 1 >"$D/all_1"
record_get all 0 >"$D/all_0"

record_get stdout 1 >"$D/stdout_1"
record_get stdout 0 >"$D/stdout_0"

record_get stderr 1 >"$D/stderr_1"
record_get stderr 0 >"$D/stderr_0"

# Stop recording after retrieving to also test that output buffering does not
# affect data retrieval.
record stop

# Check if final data is same as initial data
record_get all 1 >"$D/stop"

yaml "log: |"
yaml_file "$D/stop" 2

diff -u "$D/stop" "$D/all_1" >/dev/null
ok $? "stop"

# Check captured data
check_file "$D/defaults" 1 1 1
check_file "$D/all_1" 1 1 1
check_file "$D/all_0" 1 1 0
check_file "$D/stdout_1" 1 0 1
check_file "$D/stdout_0" 1 0 0
check_file "$D/stderr_1" 0 1 1
check_file "$D/stderr_0" 0 1 0

exit $(exit_status)
