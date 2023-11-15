#!/bin/bash
#
# Source this file to unset all tela-specific variables. This may be required
# to prevent side-effects in tests implemented as tela-based sub-Makefiles.
#

unset V PRETTY SCOPE LOG DATA COLOR CACHE BEFORE AFTER SKIPFILE RUNLOG
unset MAKEFLAGS TESTS

for VAR in $(env) ; do
	VAR="${VAR%%=*}"
	[[ "$VAR" =~ ^_?TELA ]] && unset "$VAR"
done

# Ensure defined .telarc
export TELA_RC=/dev/null
