#!/bin/bash
#
# Check if matching works with specific resource and requirement combinations
#

source $TELA_BASH || exit 1

# Remove leading space
function strip_space() {
	local var="$1" val indent

	eval "val=\$$var"
	indent="${val%%[![:space:]]*}"
	val=${val#"$indent"}
	eval "$var='$val'"
}

IFS=$'\n'
OUT=$TELA_TMP/out
TRUE=$(type -P true)

for YAML in data/*.yaml ; do
	BASE=$(basename $YAML .yaml)
	EXEC="$TELA_TMP/$BASE"
	MATCH="^ok[^#]*$"
	RC=double.rc

	# Create test program
	cat >"$EXEC" <<'EOF'
#!/bin/bash

RC=0

echo "Variables:"
set | grep TELA_SYSTEM

EOF
	chmod u+x "$EXEC"

	while read -r LINE ; do
		case "$LINE" in
		"# test:"*)
			COND="${LINE#*: }"
			strip_space COND
			cat >>"$EXEC" <<EOF
if $COND ; then
	echo '$COND' passed
else
	echo '$COND' failed
	RC=1
fi
EOF
			;;
		"# result:"*)
			MATCH="${LINE#*: }"
			strip_space MATCH
			;;
		"# rc:"*)
			RC="${LINE#*: }"
			strip_space RC
			;;
		*) ;;
		esac
	done <$YAML

	yaml "rcfile: $RC"
	yaml "match: $MATCH"

	cat >>"$EXEC" <<'EOF'
exit $RC
EOF
	cp $YAML "$TELA_TMP/"

	TELA_RC=$(pwd)/$RC TELA_VERBOSE=0 $TELA_TOOL run "$EXEC" >$OUT 2>&1

	yaml "output: |"
	yaml_file $OUT 2

	yaml "expect_result: \"$MATCH\""

	grep -q "$MATCH" $OUT
	ok $? $BASE
done

exit $(exit_status)
