#!/bin/bash
#
# Check if 'tela run' correctly processes command output when command output
# chunks are larger than internal blocking size.
#

TELA="$TELA_FRAMEWORK/src/tela"
CMD="$TELA_TMP/cmd"
OUT="$TELA_TMP/out"

# Create test command that creates large output without delay
echo "#!/bin/bash" >>$CMD
echo "cat <<EOF" >>$CMD
i=0
while [ $i -lt 1000 ] ; do
	printf "This is line %04d\n" $i
	let i=$i+1
done >>$CMD
echo "EOF" >>$CMD
chmod u+x $CMD

# Filter through 'tela run'
$TELA run $CMD | grep stdout >$OUT

echo "Output"
cat $OUT

NUM=$(wc -l < $OUT)
if [ $NUM -ne 1000 ] ; then
	echo "Error: Got $NUM lines, expected 1000" >&2
	exit 1
fi

exit 0
