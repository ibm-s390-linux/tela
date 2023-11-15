#!/bin/bash
#
# Test standalone compilation of framework header files
#

source $TELA_BASH || exit 1

for HEADER in $TELA_BASE/src/*.h ; do
	BASE=${HEADER##*/}
	gcc -x c -c -o /dev/null - >/dev/null 2>$TELA_TMP/out << EOF
#include "$HEADER"

int main(int argc, char *argv[])
{
	return 0;
}
EOF

	RC=$?
	yaml "rc:"
	yaml "  expect: 0"
	yaml "  actual: $RC"

	[ $RC -ne 0 ] && RC=1

	yaml "stderr:"
	yaml "  expect: \"\""
	if [ -s $TELA_TMP/out ] ; then
		yaml "  actual: |"
		yaml_file $TELA_TMP/out 2
		RC=1
	else
		yaml "  actual: \"\""
	fi

	ok $RC "$BASE"
done
