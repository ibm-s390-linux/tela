#!/bin/bash

source "$TELA_BASH" || exit 1

telarc="$TELA_TMP/telarc"
cat >"$telarc" <<EOF
dummy 1_does_not_exist:
EOF
logfile="$TELA_TMP/log"
cmake="./clear_make.sh"
exe="/bin/true"

record all

# Check if 'make check' writes warning about missing resource to test log file
"$cmake" check TESTS="$exe" TELA_RC="$telarc" LOG="$logfile" PRETTY=0
grep "^# WARNING.*Resource unavailable.*does_not_exist" "$logfile"
ok $? "basic"

# Check if 'make check BEFORE=...' writes warning about missing resource to test
# log file
"$cmake" check TESTS="$exe" BEFORE="$exe" TELA_RC="$telarc" LOG="$logfile" PRETTY=0
grep "^# WARNING.*Resource unavailable.*does_not_exist" "$logfile"
ok $? "before"

# Check if 'make check AFTER=...' writes warning about missing resource to test
# log file
"$cmake" check TESTS="$exe" AFTER="$exe" TELA_RC="$telarc" LOG="$logfile" PRETTY=0
grep "^# WARNING.*Resource unavailable.*does_not_exist" "$logfile"
ok $? "after"

exit 0
