#!/bin/bash

source "$TELA_BASH" || exit 1

FILTER="$TELA_FRAMEWORK/src/libexec/filter"
OUT="$TELA_TMP/out"
IN='filter_pci_ids.in'
IN2="$TELA_TMP/filter_pci_ids.in"

# Filter out comments for easier comparison of expect vs. actuals
sed -e '/^\s*#/d' "$IN" > "$IN2"

# Run test data through filter
record all
"$FILTER" "$IN2" 1
RC=$?
record stop
record_get all 0 >"$OUT"

ok $RC "rc"

# Compare actual vs.expected output
record all
diff -u "filter_pci_ids.expect" "$OUT"
RC=$?
record stop

ok $RC "output"

exit 0
