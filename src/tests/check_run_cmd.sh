#!/bin/bash
#
# Check if the check_run_cmd Bash API function correctly processes a tool's
# stdout and stderr output, and exit code.
#
set -u

source "$TELA_BASH" || exit 1

TOOL="./run_cmd_ex.sh"
TOOL2="./run_cmd_args.sh"

check_run_cmd -n 'defaults' -- "$TOOL" 0 1 1
file="${TELA_TMP}/file with white spaces"
rm -f -- "$file"
check_run_cmd -n "quote_white_space1" touch "$file"
[ -f "$file" ]
ok $? 'quote_white_space2'
check_run_cmd -n 'status_code' -s 3 -- "$TOOL" 3 1 1
check_run_cmd -n 'stdout_stderr_exact_match' -s 0 -o '^stdout$' -e '^stderr$' -f "${TELA_TMP}/here" -- "$TOOL" 0 1 1
check_run_cmd -s 0 -n 'ignore_stdout_stderr' -o '.*' -e '.*' -- "$TOOL" 0 1 0
stdout_path="${TELA_TMP}/stdout_test"
stderr_path="${TELA_TMP}/stderr_test"
check_run_cmd -n 'paths_are_used' -f "${stdout_path}" -g "${stderr_path}" -- "$TOOL" 0 1 1
diff -u <(echo -n 'stdout') "$stdout_path" &>/dev/null
ok $? 'paths_are_used_stdout'
diff -u <(echo -n 'stderr') "$stderr_path" &>/dev/null
ok $? 'paths_are_used_stderr'
printf "%b" 'bli\nbla' |check_run_cmd -n 'stdin' -o '^bli
bla$' -e '' -- 'cat'

check_run_cmd -n 'stdin_here_document' -o '^bli\nbla\n$' -e '' -- cat <<EOF
bli
bla
EOF

check_run_cmd -n 'multiline_pattern_test' -o '^multiline\npattern$' -- "$TOOL" 0 1 0 'multiline\npattern'
output="$(cat <<EOF
General dump info:
  Dump format........: elf
  Version............: 1
  System arch........: s390x (64 bit)
  CPU count (online).: 1024
  Dump memory range..: 0 MB

Memory map:
  0000000000000000 - 000000000000ffff (0 MB)
  0000000000020000 - 0000000000020fff (0 MB)
EOF
	)"
output_regex="$(cat <<EOF
General dump info:
  Dump format\.\.\.\.\.\.\.\.: elf
  Version\.\.\.\.\.\.\.\.\.\.\.\.: 1
  System arch\.\.\.\.\.\.\.\.: s390x \(64 bit\)
  CPU count \(online\)\.: 1024
  Dump memory range\.\.: 0 MB

Memory map:
  0000000000000000 - 000000000000ffff \(0 MB\)
  0000000000020000 - 0000000000020fff \(0 MB\)
EOF
	)"
check_run_cmd -n 'multiline_pattern_test2' -o "^${output_regex}$" -e '' -- "$TOOL" 0 1 0 "$output"

output="hallo 'test '\''message'\'' '"
check_run_cmd -n 'quoting_test' -o "^hallo 'test '\\\''message'\\\'' '\$" -- printf '%s' "$output"

exit "$(exit_status)"
