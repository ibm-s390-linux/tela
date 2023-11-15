#!/bin/bash
#
# Check if the log_file function works correctly
#

source $TELA_BASH || exit 1

exec="${TELA_EXEC##*/}"
tmp_dir="$_TELA_FILE_ARCHIVE/tela_tmp"
test_dir_base="$_TELA_FILE_ARCHIVE/${TELA_EXEC#$TELA_TESTBASE/}"

function atresult_callback {
	if [[ "$1" = "log_file" ]]; then
		log_file "$TELA_EXEC" "atresult.sh"
	fi
}
atresult atresult_callback NULL

log_file "$TELA_EXEC"
log_file "$TELA_EXEC" "testrename.sh"
log_file "$TELA_EXEC" "testrename.sh"

if [[ -f "$tmp_dir/$exec" && -f "$tmp_dir/testrename.sh" && -f "$tmp_dir/testrename.sh.2" ]]; then
	pass "log_file"
else
	yaml "reason: \"At least one of the three copied files does not exist\""
	fail "log_file"
fi

if [[ -f "$test_dir_base/log_file/$exec" && -f "$test_dir_base/log_file/testrename.sh" && -f "$test_dir_base/log_file/testrename.sh.2" ]]; then
	pass "move_files"
else
	yaml "reason: \"At least one of the three copied files does not exist in $test_dir_base/log_file/\""
	fail "move_files"
fi

if [[ -f "$test_dir_base/log_file/atresult.sh" ]]; then
	pass "move_files2"
else
	yaml "reason: \"The copied file from the atresult callback does not exist in $test_dir_base/log_file/\""
	fail "move_files2"
fi

exit ${exit_status}
