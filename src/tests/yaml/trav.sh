#!/bin/bash
#
# Tests for function yaml.c:yaml_traverse():
# - different input data
# - delete during traversal
# - replace during traversal
#

source $TELA_BASH || exit 1

DATADIR=trav_data

for YAML in $DATADIR/*.yaml ; do
	NAME=${YAML##*/}
	NAME=${NAME%.yaml}
	OUT=$TELA_TMP/${NAME}.out

	./yamltest traverse $YAML 2>&1 >$OUT
	RC_RUN=$?

	yaml "rc:"
	yaml "  expect: 0"
	yaml "  actual: $RC_RUN"

	diff -u ${YAML}.out $OUT >$TELA_TMP/diff
	RC_DIFF=$?

	if [ -s $TELA_TMP/diff ] ; then
		yaml "output:"
		yaml_file $TELA_TMP/diff 2
	else
		yaml "output: \"\""
	fi

	if [ $RC_RUN -ne 0 -o $RC_DIFF -ne 0 ] ; then
		fail "$NAME"
	else
		pass "$NAME"
	fi
done

exit $(exit_status)
