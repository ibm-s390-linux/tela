#!/bin/bash
#
# Ensure that standard file descriptors are available, and no other
# file descriptors are leaked to the test executable run via the run_cmd
# Bash API function.
#

source $TELA_BASH || exit 0

run_cmd "./check_fd" "run_cmd" 0 1 0

check_run_cmd -n 'check_run_cmd' -o '.*' -e '' -- './check_fd'

exit 0
