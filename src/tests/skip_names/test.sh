#!/bin/bash
#
# Check if simple tests are correctly named in case of various skip
# scenarios.
#

# Ensure stable test names when run from top-level
cd skip_names 2>/dev/null

MAKE_STDERR="$TELA_TMP/stderr"
ACTUAL="$TELA_TMP/actual.log"
ACTUAL_SHORT="$TELA_TMP/actual-short.log"
EXPECT_SHORT="expect-short.out"

echo "Get TAP13 output for sample tests"
../build_make.sh check PRETTY=0 2>"$MAKE_STDERR" >"$ACTUAL"
make clean >/dev/null

if [[ ! -s "$ACTUAL" ]] ; then
	echo "Make failed to create TAP13 output"
	cat "$MAKE_STDERR"
	exit 1
fi

echo "Filter out unimportant lines"
grep '\(^ok\|^not\|desc:\)' <"$ACTUAL" >"$ACTUAL_SHORT"

echo "Compare with expected result"

echo "Actual TAP13 log:"
sed -e 's/^/  /g' "$ACTUAL_SHORT"

echo "Expected TAP13 log:"
sed -e 's/^/  /g' "$EXPECT_SHORT"

echo "Diff:"
diff -u "$EXPECT_SHORT" "$ACTUAL_SHORT"

if [[ $? -ne 0 ]] ; then
	echo "Comparison failed"
	exit 1
fi

echo "Output as expected"

exit 0
