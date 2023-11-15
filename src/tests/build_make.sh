#!/bin/bash
#
# Helper script to run build tests in a clean environment
#

# Unset all tela-specific variables to prevent side-effects in sub-make
unset V PRETTY SCOPE LOG CACHE MAKEFLAGS FILTER RUNLOG
for VAR in $(env) ; do
	VAR=${VAR%%=*}
	[[ $VAR =~ ^_?TELA ]] && unset $VAR
done

# Ensure defined .telarc
export TELA_RC=/dev/null

make -s $*

exit $?
