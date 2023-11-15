#!/bin/bash
#
# Check if 'tela run' correctly processes commands with terminating newline.
#

TELA="$TELA_FRAMEWORK/src/tela"
OUT="$TELA_TMP/out"
CMD="$TELA_TMP/cmd"

cat >$CMD <<EOF
#!/bin/bash

echo text

exit 0
EOF
chmod u+x $CMD

# Filter through 'tela run'
$TELA run $CMD >$OUT 2>&1

echo "Output"
cat $OUT

if ! grep -q 'stdout: text' < $OUT ; then
	echo "Error: Output not as expected" >&2
	exit 1
fi

exit 0
