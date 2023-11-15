#!/bin/bash
#
# Check if 'tela match' correctly produces YAML output if requested.
#

source $TELA_BASH || exit 1

# Create resource file
RES="$TELA_TMP/res"
cat >"$RES" <<EOF
system:
  dummy dummy_1:
    val: 1
  dummy dummy_2:
    val: 2
    num: 7
EOF

# Create request file
REQ="$TELA_TMP/req"
cat >"$REQ" <<EOF
system:
  dummy a:
    val: 2
EOF

record all

# Run command
"$TELA_TOOL" match "$REQ" "$RES" 0 1
RC=$?
OUT="$TELA_TMP/out"
record_get stdout 0 >"$OUT"
ok "$RC" "match_rc"

# Check for object_id of system object
grep "^  _id: localhost" <"$OUT"
ok $? "sys_object_id"

# Check for object_id of dummy object
grep "^    _id: dummy_2" <"$OUT"
ok $? "dummy_object_id"

# Check for value attribute of dummy object
grep "^    val: 2" <"$OUT"
ok $? "dummy_val"

# Check for value attribute of dummy object
grep "^    num: 7" <"$OUT"
ok $? "dummy_num"

# Make sure only one dummy object is reported
NUM=$(grep "^  dummy" <"$OUT" | wc -l)
echo "Number of dummy objects in YAML output: $NUM"
[[ "$NUM" == 1 ]]
ok $? "num_dummys"

exit $(exit_status)
