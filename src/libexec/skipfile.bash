#
# SPDX-License-Identifier: MIT
# skipfile.bash - Bash functions that encapsulate processing of skipped tests.
#
# Copyright IBM Corp. 2023
#

function is_skip_file_defined() {
	[[ -n "$_TELA_SKIP" ]]
}

function is_test_skipped() {
	local test=$1
	local pattern

	[[ -n "$_TELA_SKIP" ]] || return 1

	while read -r pattern; do
		[[ "$test" == $pattern ]] && return 0
	done < "$_TELA_SKIP"

	return 1
}
