#!/bin/bash
#
# This script is used as test executable for the tela test plan files in
# warn_data/. Any comment between "BEGIN TAP13" and "END TAP13" is printed
# as test executable output.
#

PRINT=0

while read -r LINE ; do
	case "$LINE" in
	*"BEGIN TAP13") PRINT=1 ;;
	*"END TAP13") PRINT=0 ;;
	*) [[ "$PRINT" == "1" ]] && echo "${LINE:2}" ;;
	esac
done < "$0.yaml"

exit 0
