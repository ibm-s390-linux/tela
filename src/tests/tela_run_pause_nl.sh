#!/bin/bash
#
# Check if 'tela run' correctly processes commands which pause before emitting
# a newline: only full lines should be logged, even if the newline of a line is
# emitted after a pause.
#

TELA="$TELA_FRAMEWORK/src/tela"
OUT="$TELA_TMP/out"
CMD="$TELA_TMP/cmd"
RC=0

cat >$CMD <<EOF
#!/bin/bash

echo ab

echo -n c
echo d

echo -n e
sleep 1
echo f

echo -n g
echo err >&2
echo h

echo ij

echo -n k

exit 0
EOF
chmod u+x $CMD

# Filter through 'tela run'
$TELA run $CMD >$OUT 2>&1

echo "Output"
cat $OUT

NRSTDOUT=$(grep 'stdout:' $OUT | wc -l)
NRSTDOUTNONL=$(grep 'stdout(nonl' $OUT | wc -l)
NRSTDERR=$(grep 'stderr:' $OUT | wc -l)

echo "Got NRSTDOUT=$NRSTDOUT (expect 5)"
if [[ $NRSTDOUT != 5 ]] ; then
	echo "Error: NRSTDOUT not as expected" >&2
	RC=1
fi

echo "Got NRSTDOUTNONL=$NRSTDOUTNONL (expect 1)"
if [[ $NRSTDOUTNONL != 1 ]] ; then
	echo "Error: NRSTDOUTNONL not as expected" >&2
	RC=1
fi

echo "Got NRSTDERR=$NRSTDERR (expect 1)"
if [[ $NRSTDERR != 1 ]] ; then
	echo "Error: NRSTDERR not as expected" >&2
	RC=1
fi

exit $RC
