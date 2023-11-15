#!/bin/bash
#
# Check if current time is before noon (am) or after noon (pm).
#

# Include tela functions
source $TELA_BASH || exit 1

HOUR=$(date +%-H)

# Testcase before_noon
test $HOUR -lt 12
ok $? "am"

# Testcase after_noon
if [ $HOUR -ge 12 ] ; then
	pass "pm"
else
	fail "pm"
fi

exit $(exit_status)
