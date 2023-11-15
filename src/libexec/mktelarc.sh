#!/bin/bash
# SPDX-License-Identifier: MIT
#
# mktelarc.sh - create a template .telarc file
#
# Copyright IBM Corp. 2023
#
# Internal helper script for use by tela.mak: Create a template for a .telarc
# file containing all known resources.
#

function die() {
	echo -n "${0##*/}: "
	echo "$@" >&2
	exit 1
}

if [[ -z "$TELA_FRAMEWORK" ]] ; then
	die "This script is run automatically from tela.mak"
fi

cat <<EOF
# This is an automatically generated tela resource file containing all resources
# found on the local system. To activate it, place it in a file named .telarc
# in your home directory.
#
# WARNING: The resources listed in this file may temporarily stop working
#          during testing. Please review the list and remove those resources
#          that are required for the operation of your test system (this
#          typically includes the root device, and the main networking device)

EOF

$TELA_FRAMEWORK/src/libexec/resources/system
