#!/bin/bash
#
# Tests for function yaml.c:yaml_traverse2():
# - different input data
# - delete during traversal
# - replace during traversal
#

source $TELA_BASH || exit 1

DATADIR=trav2_data

for YAML in $DATADIR/*.a.yaml ; do
	NAME=${YAML##*/}
	NAME=${NAME%.a.yaml}
	OUT=$TELA_TMP/${NAME}.out
	BASE=$DATADIR/${NAME}

	./yamltest traverse2 ${BASE}.a.yaml ${BASE}.b.yaml 2>&1 >$OUT
	RC_RUN=$?

	yaml "rc:"
	yaml "  expect: 0"
	yaml "  actual: $RC_RUN"

	diff -u ${BASE}.out $OUT >$TELA_TMP/diff
	RC_DIFF=$?

	yaml "output:"
	yaml_file $TELA_TMP/diff 2

	if [ $RC_RUN -ne 0 -o $RC_DIFF -ne 0 ] ; then
		fail "$NAME"
	else
		pass "$NAME"
	fi
done

exit $(exit_status)
