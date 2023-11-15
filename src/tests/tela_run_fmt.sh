#!/bin/bash
#
# Check if 'tela run' correctly processes warning output with format
# string characters.
#

TELA="$TELA_FRAMEWORK/src/tela"
CMD="$TELA_TMP/cmd"

cat >$CMD <<'EOF'
#!/bin/bash

echo "TAP version 13"

i=0
while [ $i -lt 1000 ] ; do
	echo "# Line $i"
	let i=i+1
done

echo "%s:%4s:%d\n" >&2

exit 0
EOF
chmod u+x $CMD

echo "Output:"

# Filter through 'tela run'
$TELA run $CMD

exit $?
