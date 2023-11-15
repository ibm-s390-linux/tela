#!/bin/bash

source $TELA_BASH || exit 1

passed=$TELA_TMP/passed
failed=$TELA_TMP/failed

while read -r type a b ; do
	if $TELA_TOOL eval $type $a "$b" >/dev/null ; then
		echo "$type $a $b" >>$passed
	else
		echo "$type $a $b" >>$failed
	fi
done << EOF
version 1 1
version 1.0 1.0
version 1.1 > 1.0
version 2.0 >= 2
version 2.0 2
version 1.a 1.a
version 1.a < 1.b
version 1 < 1.b
version 1. < 1.b
version 1.a > 1.
version 1.a > 1
version 1.0 < 1.a
version 1.2.3 < 2.3.a
version 1.2.3 < 1.2.a
version 1.010 != 1.8
version 047 47
version 0x10 != 16
version 3e != 3
EOF

yaml "passed: |"
yaml_file "$passed" 2

if [[ -s "$failed" ]] ; then
	yaml "failed: |"
	yaml_file "$failed" 2
	fail "eval"
else
	pass "eval"
fi

exit $(exit_status)
