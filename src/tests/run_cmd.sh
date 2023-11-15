#!/bin/bash
#
# Check if the run_cmd Bash API function correctly processes a tool's
# stdout and stderr output, and exit code.
#

source $TELA_BASH || exit 1

TOOL="./run_cmd_ex.sh"
TOOL2="./run_cmd_args.sh"

# No output, exit code 0
run_cmd "$TOOL 0 0 0" "silent_0" 0 0 0

# Output on stdout, exit code 0
run_cmd "$TOOL 0 1 0" "stdout_0" 0 1 0

# Output on stderr, exit code 0
run_cmd "$TOOL 0 0 1" "stderr_0" 0 0 1

# Output on both stdout and stderr, exit code 0
run_cmd "$TOOL 0 1 1" "all_0" 0 1 1

# No output, exit code 1
run_cmd "$TOOL 1 0 0" "silent_1" 1 0 0

# Output on stdout, exit code 1
run_cmd "$TOOL 1 1 0" "stdout_1" 1 1 0

# Output on stderr, exit code 1
run_cmd "$TOOL 1 0 1" "stderr_1" 1 0 1

# Output on both stdout and stderr, exit code 1
run_cmd "$TOOL 1 1 1" "all_1" 1 1 1

# Check that parameter quoting works as expected and the environment variable
# set by `run_cmd` can be used
run_cmd "$TOOL2 --1 \"a;b\" --2 'c;d'" "quote" 0 1 0

diff -u "$TELA_RUN_CMD_STDOUT" - <<EOF >"$TELA_TMP/quote_diff"
--1
a;b
--2
c;d
EOF
RC=$?
yaml_file "$TELA_TMP/quote_diff" 0 "diff"
ok $RC quote2

# Check that parameters with white spaces are properly quoted
file="${TELA_TMP}/file with white spaces"
testname='quote_white_space1'
stdout_path="${TELA_TMP}/${testname}_stdout"
stderr_path="${TELA_TMP}/${testname}_stderr"
run_cmd "touch '$file'" "$testname" 0 0 0
[ -f "$file" ]
ok $? 'quote_white_space2'

# Check that the stdout and stderr output files are located as expected
[ -f "${stdout_path}" ]
ok $? 'stdout_file_exists'
[ -f "${stderr_path}" ]
ok $? 'stderr_file_exists'

[[ "${TELA_RUN_CMD_STDOUT:-}" == "${stdout_path}" ]] && [[ "${TELA_RUN_CMD_STDERR:-}" == "${stderr_path}" ]]
ok $? 'run_cmd_env_variables_set'

cmd="$TOOL"
cmd+=' 0 1 0 "I love eval... testname is: '"'"'$name'"'"' \npattern"'
run_cmd "$cmd" 'multiline_cmd' 0 1

exit 0
