#!/bin/bash
#
# Usage: run_cmd_ex.sh EXIT_CODE STDOUT STDERR
#
# If STDOUT is non-zero, print text to stderr stream. If STDERR is non-zero,
# print text to stderr stream. Exit with exit code EXIT_CODE.
#
# This tool is used to exercise the run_cmd API function.
#

EXIT_CODE="${1:-0}"
STDOUT="${2:-0}"
STDERR="${3:-0}"
STDOUTV="${4:-stdout}"
STDERRV="${5:-stderr}"

[[ "$STDOUT" != "0" ]] && printf "%b" "$STDOUTV"
[[ "$STDERR" != "0" ]] && printf "%b" "$STDERRV" >&2

exit $EXIT_CODE
