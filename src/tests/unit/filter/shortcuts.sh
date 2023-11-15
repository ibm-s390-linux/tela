#!/bin/bash
#
# Check if filter script correct accounts for indentation level when
# expanding shortcuts
#

source "$TELA_BASH" || exit 1

FILTER="$TELA_FRAMEWORK/src/libexec/filter"
IN='shortcuts.in'
IN2="$TELA_TMP/in"
OUT="$TELA_TMP/out"
ERR="$TELA_TMP/err"
UNCHANGED="$TELA_TMP/unchanged"
CHANGED="$TELA_TMP/changed"

# Filter out comments and empty lines from input file
sed -e '/^$/d' -e '/^\s*#/d' < "$IN" > "$IN2"

# Run test data through filter
record all
"$FILTER" "$IN2" 1
RC=$?
record stop
record_get stdout 0 >"$OUT"
record_get stderr 0 >"$ERR"

# Check for unexpected return code != 0
ok $RC "rc"

# Check for unexpected warnings on stderr
yaml_file "$ERR" 0 'warnings'
! [[ -s "$ERR" ]]
ok $? "no_warnings"

# Determine changed and unchanged lines
diff "$IN2" "$OUT" --old-line-format '' --new-line-format '' \
     --unchanged-line-format '%L' >"$UNCHANGED"
diff "$IN2" "$OUT" --old-line-format '%L' --new-line-format '' \
     --unchanged-line-format '' >"$CHANGED"

# Check if any must-expand shortcut was not expanded
record stdout
grep 'doexpand' "$UNCHANGED"
RC=$?
record stop
yaml_file "$UNCHANGED" 0 'unchanged'
[[ "$RC" -ne 0 ]]
ok $? 'missing_expand'

# Check if any shortcut that must be skipped was expanded
record stdout
grep 'noexpand' "$CHANGED"
RC=$?
record stop
yaml_file "$CHANGED" 0 'changed'
[[ "$RC" -ne 0 ]]
ok $? 'invalid_expand'

exit 0
