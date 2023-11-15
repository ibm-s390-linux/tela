#!/bin/bash
# SPDX-License-Identifier: MIT
#
# log_file.sh - tool for implementing log_file() API function
#
# Copyright IBM Corp. 2023
#
# Internal helper script for Tela:
# Implements core functionality of the function log_files.
#

function die() {
	echo "${0##*/}: $@" >&2
	exit 1
}

function warn() {
	echo "${0##*/}: $@" >&2
}

function log_file() {
	local tmp_dir="$_TELA_FILE_ARCHIVE/tela_tmp"
	local file="$1"
	local name="$2"
	local final_name="$name"
	local i="2"

	if [[ ! -d "$tmp_dir" ]]; then
		mkdir -p "$tmp_dir"
	fi

	while [[ -f "$tmp_dir/$final_name" ]]; do
		final_name="$name.$i"
		((i++))
	done

	cp -f "$file" "$tmp_dir/$final_name" || warn "Failed to log file $file"
}

# moves additional data to correct path after test
function move_files() {
	local testname="$1"
	local tmp_dir="$_TELA_FILE_ARCHIVE/tela_tmp"
	local test_dir_base="$_TELA_FILE_ARCHIVE/${TELA_EXEC#$TELA_TESTBASE/}"
	local test_dir="$testname"
	local i="2"

	if [[ -d "$tmp_dir" ]]; then
		if [[ ! -d "$test_dir_base" ]]; then
			mkdir -p "$test_dir_base"
		fi

		while [[ -d "$test_dir_base/$test_dir" ]]; do
			test_dir="$testname.$i"
			((i++))
		done

		mv -f "$tmp_dir" "$test_dir_base/$test_dir" || warn "Failed to move logged files"
	fi
}

if [[ -z "$_TELA_FILE_ARCHIVE" ]] ; then
	die "This script is run automatically when log_files is called by a test"
fi

if [[ "$1" = "log_file" ]]; then
	log_file "$2" "$3"
elif [[ "$1" = "move_files" ]]; then
	move_files "$2"
else
	die "Unknown command"
fi

exit 0
