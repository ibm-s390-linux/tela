#!/bin/bash
#
# Check warning output of YAML parser
#

source $TELA_BASH || exit 1

OUT=$TELA_TMP/out
TOOL="$PWD/mktap13.sh"

cd warn_data
for YAML in *.yaml ; do
	BASE=$(basename $YAML .yaml)
	EXPECT=$(grep Expect: $YAML | cut -d: -f2)

	cp $TOOL $BASE
	$TELA_TOOL run $BASE 2>/dev/null | grep '^# WARNING' | grep -v Resource | cut -c 2- >$OUT

	yaml "expect: \"$EXPECT\""
	if [ -s "$OUT" ] ; then
		yaml "actual: |"
	else
		yaml "actual: \"\""
	fi
	yaml_file $OUT 2

	if [ -n "$EXPECT" ] ; then
		# Check for expected warning
		grep "$EXPECT" $OUT >/dev/null
	else
		# Should be empty
		test ! -s $OUT
	fi
	ok $? $BASE
	rm -f $BASE
done

exit $(exit_status)
