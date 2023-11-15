#!/bin/bash
#
# Check if TELA_BASH is set. No use to continue testing if it is not set since
# no Bash based tests would run.
#

RC=0

echo "TAP version 13"
echo "1..1"

if [ -n "$TELA_BASH" ] ; then
	echo "ok tela_bash"
else
	echo "Bail out! TELA_BASH is not set"
	RC=1
fi

cat <<EOF
  ---
    TELA_BASH: "$TELA_BASH"
  ...
EOF


exit $RC
