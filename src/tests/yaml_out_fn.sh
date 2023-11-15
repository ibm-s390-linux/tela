function check_output() {
	local filename="$1" expected="$2" filtered="$TELA_TMP/filtered"

	# Filter out irrelevant portions of TAP output
	grep '^  ' "$filename" | grep -v "^\s\+\(CC\|testresult\|testexec\|starttime\|stoptime\|duration_ms\|source\)" >$filtered

	echo "Expected output:"
	cat "$expected"

	echo
	echo "Actual output:"
	cat "$filtered"

	echo
	echo "Diff:"
	diff -u "$expected" "$filtered"
}

function fail() {
	echo "Error: $*" >&2
	exit 1
}

