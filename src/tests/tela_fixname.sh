#!/bin/bash
#
# Ensure that tela's fixname function works correctly.
#

RC=0
IFS=$'\n'
while read -r testname ; do
	read expect
	actual=$($TELA_TOOL fixname "$testname") || exit 1

	echo "Checking testname '$testname'"
	echo "  expect ..: '$expect'"
	echo "  actual ..: '$actual'"

	if [[ "$actual" != "$expect" ]] ; then
		echo "  result ..: failed"
		RC=1
	else
		echo "  result ..: passed"
	fi
done <<'EOF'
abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789._
abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789._
	 ^!"§$%&/()=?`´*+#':;,
_
$
_


$a$b$
a_b
$$a$$b$$
a_b
	some name with    spaces	
some_name_with_spaces
-name-
-name-
EOF

exit $RC
