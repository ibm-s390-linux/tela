#!/bin/bash
# SPDX-License-Identifier: MIT
#
# mkplan.sh - create test plans for TAP13 log
#
# Copyright IBM Corp. 2023
#
# Internal helper script for use by tela.mak: Create test plans from TAP13
# output found in LOG. Plan will be generated in <testexec>.yaml.new.
#

# Use default name if none was provided by caller
[[ -z "$LOG" ]] && LOG="test.log"

if [[ ! -e "$LOG" ]] ; then
	echo "Error: Could not open log file $LOG - run 'make check' first" >&2
	exit 1
fi

if [[ -t 1 ]] ; then
	BOLD="\033[1m"
	BLUE="\033[1;34m"
	RED="\033[1;31m"
	RESET="\033[0m"
fi

echo -e "${BOLD}Auto-generating test plans from $LOG:$RESET"

LAST=
export IFS=-
while read -r a b ; do
	# Only consider lines containing test results
	[[ "$a" =~ ^(ok|not ok) ]] || continue

	# Extract filename and testname
	b=${b# *}
	FILE=${b%:*}
	TEST=${b#*:}

	# Start new file if necessary
	if [[ -z "$LAST" ]] || [[ "$LAST" != "$FILE" ]] ; then
		LAST="$FILE"
		FILENAME="$FILE.yaml.new"
		if [[ -e "$FILENAME" ]] ; then
			echo -e "Skipping $BLUE$FILENAME $RED(file already exists)$RESET"
			FILENAME="/dev/null"
		else
			echo -ne "Creating $BLUE$FILENAME$RESET "
			if [[ -e "$FILE.yaml" ]] ; then
				echo -e "(add to $BOLD$FILE.yaml$RESET to activate)"
			else
				echo -e "(rename to $BOLD$FILE.yaml$RESET to activate)"
			fi

			echo "test:" >$FILENAME
			echo "  plan:" >>$FILENAME
			echo "    # Format is <testname>: \"<description>\"" >>$FILENAME
		fi
	fi
	echo "    $TEST: \"\"" >>"$FILENAME"
done <"$LOG"
