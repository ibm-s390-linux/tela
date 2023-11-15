#!/bin/bash
# SPDX-License-Identifier: MIT
#
# mkstate.sh - print status of all defined resources
#
# Copyright IBM Corp. 2023
#
# Internal helper script for use by tela.mak: Collect and print the status of
# all resources defined in the .telarc file.
#

function die() {
	echo -n "${0##*/}: "
	echo "$@" >&2
	exit 1
}

if [[ -z "$TELA_FRAMEWORK" ]] ; then
	die "This script is run automatically from tela.mak"
fi

TMPFILE=$(mktemp) || die "Could not create temporary file"
trap "rm -f '$TMPFILE'" EXIT

$TELA_FRAMEWORK/src/libexec/resources/system >"$TMPFILE"
$TELA_FRAMEWORK/src/libexec/resources/system "$TMPFILE"
