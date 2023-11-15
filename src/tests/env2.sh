#!/bin/bash
#
# Check for environment variables provided by framework.
#

source $TELA_BASH || exit 1

# Check for TELA_OS_ID
yaml "TELA_OS_ID: \"$TELA_OS_ID\""
test -n "$TELA_OS_ID"
ok $? "tela_os_id"

# Check for TELA_OS_VERSION
yaml "TELA_OS_VERSION: \"$TELA_OS_VERSION\""
test -n "$TELA_OS_VERSION"
ok $? "tela_os_version"

# Check for TELA_TMP
yaml "TELA_TMP: \"$TELA_TMP\""
if [ -n "$TELA_TMP" ] ; then
	pass "tela_tmp"

	# Check if temp directory is empty
	test -z "$(shopt -s nullglob dotglob; s=($TELA_TMP/*); echo ${s[*]})"
	ok $? "tela_tmp_empty"

	# Check if temp directory is writable
	echo test > "$TELA_TMP/testfile"
	ok $? "tela_tmp_writable"
else
	fail "tela_tmp"
	skip "tela_tmp_empty"
	skip "tela_tmp_writable"
fi

# Check for TELA_EXEC
yaml "TELA_EXEC: \"$TELA_EXEC\""
test -n "$TELA_EXEC"
ok $? "tela_exec"

# Check for TELA_SCOPE
yaml "TELA_SCOPE: \"$TELA_SCOPE\""
test -n "$TELA_SCOPE"
ok $? "tela_scope"

# Check for TELA_SYSTEM
yaml "TELA_SYSTEM: \"$TELA_SYSTEM\""
test -n "$TELA_SYSTEM"
ok $? "tela_system"

exit $(exit_status)
