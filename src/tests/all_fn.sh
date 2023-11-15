function check_output {
	local filename="$1" expect="$2"

	# Determine sequence of relevant output lines
	IFS=$'\n'
	unset sequence
	while read -r line ; do
		t=""
		case "$line" in
		ok*\#\ SKIP\ reason) t=S ;;
		not\ ok*)	t=F ;;
		esac
		printf "%1s:%s\n" "$t" "$line"
		sequence="${sequence}$t"
	done <$filename

	echo "Got sequence='$sequence', expected '$expect'"

	test "$sequence" == "$expect"
}
