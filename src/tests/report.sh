#!/bin/bash
#
# Check if the tela C-API can handle NULL for test names and format strings
# for all report functions.
#

D=subtests

make -C $D report || exit 1
$D/report >$TELA_TMP/out

echo "Output:"
cat $TELA_TMP/out

grep -v '^ ' < $TELA_TMP/out >$TELA_TMP/out2

echo "Diff:"
diff -u --ignore-space-change $D/report.out $TELA_TMP/out2
if [ $? -ne 0 ] ; then
	echo "Error: Output not as expected" >&2
	exit 1
fi

exit 0
