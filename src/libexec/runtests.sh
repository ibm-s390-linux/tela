#!/bin/bash
# SPDX-License-Identifier: MIT
#
# runtests.sh - run tests specified on command-line
#
# Copyright IBM Corp. 2023
#
# Internal helper script for use by tela.mak: Run tests specified on command
# line and recurse into specified sub-directories.
#

set -o pipefail

MAKE="$1"
shift
TESTS=( "$@" )

source "$TELA_FRAMEWORK/src/libexec/skipfile.bash" || exit 1

declare -r EXIT_FAIL="${EXIT_FAIL:-1}"

function die() {
	echo -n "${0##*/}: " >&2
	echo "$@" >&2
	exit 1
}

function checkscripts() {
	local v="$1" scripts="$2" script IFS

	[[ -z "$scripts" ]] && return

	IFS=":"
	for script in $scripts ; do
		if ! type -P "$script" >/dev/null ; then
			die "Cannot find command '$script' specified via $v"
		fi
	done

}

function runscripts() {
	local scripts="$1" script IFS out="$_TELA_TMPDIR/script.out" line

	[[ -z "$scripts" ]] && return
	shift

	IFS=":"
	for script in $scripts ; do
		if [[ ! -x "$script" ]] ; then
			die "Could not run '$script'"
		fi
		"$script" "$@"
	done >"$out" 2>&1

	unset IFS
	while read -r line ; do
		echo "# $line"
	done <"$out"
}

function runtests() {
	local t tests="$*" abs testdir matchearly=0 matchout=""  matcherr=""
	local rc err last

	# Determine test directory relative to test base directory
	[[ "$PWD" != "$TELA_TESTBASE" ]] && testdir="${PWD##$TELA_TESTBASE/}/"

	if [[ -n "$TELA_BEFORE" ]] || [[ -n "$TELA_AFTER" ]] ; then
		matchearly=1
	fi

	for t in $tests ; do
		# Filter out tests on the skip list
		is_test_skipped "$testdir$t" && continue

		if [[ -d "$t" ]] ; then
			# Enter sub-directory
			$MAKE -C "$t" check
		else
			abs="$PWD/$t"
			abs="${abs##$TELA_TESTBASE/}"

			if [[ "$matchearly" -eq 1 ]] ; then
				# Obtain resource match data for use in scripts
				matchout="$_TELA_TMPDIR/runtests.matchout"
				matcherr="$_TELA_TMPDIR/runtests.matcherr"
				TELA_DEBUG=0 "$TELA_TOOL" match "${t}.yaml" "" 1 \
					>"$matchout" 2>"$matcherr"
				rc=$?
				readarray -t err <"$matcherr"

				if [[ "$rc" -ne 0 ]] ; then
					# No match, use last line of stderr
					# as reason text
					last=$(( ${#err[@]}-1 ))
					if [[ "$last" -ge 0 ]] ; then
						matcherr="${err[$last]}"
						unset "err[$last]"
					else
						matcherr="Unknown match error"
					fi
					matchout=""
					if [[ ! "$matcherr" =~ ^Missing ]] ; then
						echo "$matcherr" >&2
						exit 1
					fi
				else
					matcherr=""
				fi

				# Make sure warnings are passed through
				[[ ${#err[@]} -gt 0 ]] && printf "%s\n" "${err[@]}" >&2
			fi

			# Run test
			runscripts "$TELA_BEFORE" "$TELA_TESTSUITE" "$abs" \
				   "$matchout" "$matcherr"
			$TELA_TOOL run "$t" "" "$matchout" "$matcherr" \
				   </dev/null || exit 1
			runscripts "$TELA_AFTER" "$TELA_TESTSUITE" "$abs" \
				   "$matchout" "$matcherr"
		fi
	done
}

function cleanup() {
	local s

	[[ -z "$_TELA_TMPDIR" ]] && return

	# Close SSH control sockets opened by remote script
	for s in "$_TELA_TMPDIR"/ctl_path.* ; do
		ssh -q -O exit -o "ControlPath $s" user@host 2>/dev/null
	done

	# Remove temporary directory
	rm -rf "$_TELA_TMPDIR"
}

if [[ -z "$MAKE" || -z "$TELA_TOOL" ]] ; then
	die "This script is run automatically from tela.mak"
fi

if [[ -z "$_TELA_RUNNING" ]] ; then
	# Do this only once at start of test run
	checkscripts "BEFORE" "$TELA_BEFORE"
	checkscripts "AFTER" "$TELA_AFTER"

	export _TELA_RUNNING=1
	_TELA_TMPDIR=$(mktemp -d) || die "Could not create temporary directory"
	trap cleanup exit
	export _TELA_TMPDIR
	export _TELA_FILE_ARCHIVE="$_TELA_TMPDIR/archive"

	# Announce run-log
	if [[ -n "$TELA_RUNLOG" ]] ; then
		# Reset here since each tela process only appends to this log
		echo -n 2>/dev/null >"$TELA_RUNLOG" ||
			die "Could not create run-log $TELA_RUNLOG"
		echo "Writing unprocessed test output to $TELA_RUNLOG"
	fi

	# Get total number of tests
	NUM=$($MAKE --no-print-directory count TESTS="${TESTS[*]}")
	if [[ "${#TESTS[@]}" -eq 0 ]] ; then
		P="$PWD/Makefile"
		P=${P##$TELA_TESTBASE/}
		die "$P: Empty TESTS variable"
	fi

	runtests "${TESTS[@]}" | $TELA_TOOL format - "$NUM" 1 || exit 1
	if [[ -d "$_TELA_FILE_ARCHIVE" ]]; then
		tar -czf "$TELA_WRITEDATA" -C "$_TELA_FILE_ARCHIVE" .
		echo "Additional data was stored in $TELA_WRITEDATA"
	fi

	if grep -q '^not ok' "${TELA_WRITELOG}"; then
		echo 'Tests have failed' >&2
		exit "${EXIT_FAIL}"
	fi
else
	runtests "${TESTS[@]}" || exit 1
fi

exit 0
