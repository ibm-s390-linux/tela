#!/bin/bash
# SPDX-License-Identifier: MIT
#
# counttests.sh - count number of scheduled test cases
#
# Copyright IBM Corp. 2023
#
# Internal helper script for use by tela.mak: Count tests specified on command
# line, including those defined in specified sub-directories.
#

MAKE="$1"
shift
TESTS=$*

source "$TELA_FRAMEWORK/src/libexec/skipfile.bash" || exit 1

function die() {
	echo -n "${0##*/}: "
	echo "$@" >&2
	exit 1
}

function filter_tests() {
	local testdir tests testname

	testdir="$1"
	shift
	tests="$*"

	for testname in $tests ; do
		is_test_skipped "$testdir$testname" || echo "$testname "
	done
}

function counttests() {
	local count t tests="$*" testdir

	if is_skip_file_defined ; then
		# Determine test directory relative to test base directory
		[[ "$PWD" != "$TELA_TESTBASE" ]] &&
			testdir="${PWD##$TELA_TESTBASE/}/"

		# Filter out tests on the skip list
		tests=$(filter_tests "$testdir" $tests)
	fi

	count=$($TELA_TOOL count $tests)
	for t in $tests ; do
		[[ ! -d "$t" ]] && continue

		# Count tests in sub-directory
		((count=count+$($MAKE --no-print-directory count -C "$t")))
	done

	echo $count
}

if [[ -z "$MAKE" || -z "$TELA_TOOL" ]] ; then
	die "This script is run automatically from tela.mak"
fi

if [[ -z "$_TELA_COUNTING" ]] ; then
	# Do this only once
	export _TELA_COUNTING=1

	# Ensure that tela tool and generated test YAML files are present
	$MAKE $TELA_TOOL all_check TESTS="$TESTS" >/dev/null
fi

counttests $TESTS
