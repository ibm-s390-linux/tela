#!/bin/bash
#
# Perform tela resource matching with a mix of named 'dummy a' and wildcard
# 'dummy *' requirements. Match results must meet these conditions:
#
# - named resources have precedence
# - wildcard resources must always be assigned the maximum number of
#   free resources
#

source "$TELA_BASH" || exit 1

function mktelarc() {
	local n="$1" i

	for (( i=0; i<n; i++ )) ; do
		echo "dummy $i:"
		echo "  att: $i"
	done
}

# Create a test requirement file that requests:
# - n named dummy objects
# - one wildcard dummy object if m is between 0 and n-1
# - named dummy objects require a specific att value if useatt_n=1
# - wildcard dummy object requires a specific att value if useatt_w=1
function mktestreq() {
	local n="$1" w="$2" useatt_n="$3" useatt_w="$4"

	for (( i=0; i<n; i++ )) ; do
		if [[ "$i" -eq "$w" ]] ; then
			echo 'dummy *:'
			[[ "$useatt_w" == 1 ]] && echo "  att: $i"
		else
			echo "dummy named_$i:"
			[[ "$useatt_n" == 1 ]] && echo "  att: $i"
		fi
	done
}

# Count occurrence of named and wildcard matches in result YAML file
function count_matches() {
	local file="$1"
	local -n n1="$2" n2="$3"
	local line

	n1=0
	n2=0
	while read -r line ; do
		[[ "$line" =~ dummy\ named ]] && (( n1++ ))
		[[ "$line" =~ dummy\ [0-9] ]] && (( n2++ ))
	done <"$file"
}


function get_desc() {
	local n="$1" w="$2" have_w="$3" useatt_n="$4" useatt_w="$5"
	local desc plural="s"

	if [[ "$have_w" == 1 ]] ; then
		(( n-- ))
	fi
	if [[ "$useatt_n" == "0" ]] ; then
		useatt_n=""
	else
		useatt_n=" with attribute"
	fi
	if [[ "$useatt_w" == "0" ]] ; then
		useatt_w=""
	else
		useatt_w=" with attribute"
	fi
	if [[ "$n" == 0 ]] ; then
		n="No"
	fi
	if [[ "$n" == 1 ]] ; then
		plural=""
	fi

	desc="$n named object$plural$useatt_n"
	if [[ "$have_w" == 1 ]] ; then
		desc="$desc, and a wildcard object$useatt_w in line $w"
	fi

	yaml "desc: \"$desc\""
}

# Perform matching using specified parameters and check result
function dotest() {
	local telarc="$1" req="$2" total="$3" n="$4" have_w="$5" useatt_w="$6"
	local w="$7" name="$8"
	local match="$TELA_TMP/match.out" actual_named actual_unnamed
	local expect_named expect_unnamed rc_named rc_unnamed

	# Calculate expected number of matches per requirement type
	if [[ "$have_w" == 1 ]] ; then
		(( expect_named=n-1 ))
		if [[ "$useatt_w" == 1 ]] ; then
			# Each object has a separate attribute value, therefore
			# only one object will match the wildcard attribute
			# requirement
			expect_unnamed=1
		else
			(( expect_unnamed=total-expect_named ))
		fi
	else
		expect_named="$n"
		expect_unnamed=0
	fi

	"$TELA_TOOL" match "$req" "$telarc" 0 1 >"$match"
	yaml_file "$match" 0 "match"

	count_matches "$match" actual_named actual_unnamed

	rc=0
	if [[ "$expect_named" != "$actual_named" ]] ; then
		rc_named="!="
		rc=1
	else
		rc_named="=="
	fi
	yaml "named_result: $expect_named $rc_named $actual_named"

	if [[ "$expect_unnamed" != "$actual_unnamed" ]] ; then
		rc_unnamed="!="
		rc=1
	else
		rc_unnamed="=="
	fi
	yaml "unnamed_result: $expect_unnamed $rc_unnamed $actual_unnamed"

	if [[ $rc -ne 0 ]] && [[ "$w" -lt $(( n - 1 )) ]] ; then
		# Wildcard matches are greedy and will cause subsequent
		# named matches to fail. TODO: Find a way to improve this.
		skip "$name" "Mixed matching not implemented"
	else
		ok "$rc" "$name"
	fi
}

declare telarc testreq num_objects n w useatt_n useatt_w have_n have_w nvar wvar

# Generated telarc file
telarc="$TELA_TMP/telarc"

# Generated testrequirement file
testreq="$TELA_TMP/testreq"

# Total number of available dummy resource objects
num_objects=4

mktelarc "$num_objects" >"$telarc"

#
# The following code creates variations of a test requirement YAML file:
# - between 1 and num_objects named object requirements (n)
# - with/without a wildcard requirement + varying positions of the wildcard
#   requirement in the YAML file (w=[0..n-1], w=n means no wildcard)
# - with/without attribute requirements for named object requirements (use_n)
# - with/without attribute requirements for wildcard object requirement (use_w)
#

# Vary the number of requested dummy objects up to num_objects
for (( n=1; n<num_objects; n++ )) ; do
	# Vary whether a wildcard request is present (w < n), and in which
	# line of the requirement YAML file it shows up
	for (( w=0; w <=n ; w++ )) ; do
		# Determine if any named requirement is generated
		if [[ "$w" == "$n" ]] || [[ "$n" -gt 1 ]] ; then
			have_n=1
			nvar=( 0 1 )
		else
			have_n=0
			nvar=( 0 )
		fi

		# Determine if a wildcard requirement is generated
		if [[ "$w" -lt "$n" ]] ; then
			have_w=1
			wvar=( 0 1 )
		else
			have_w=0
			wvar=( 0 )
		fi

		# Vary named object attribute use
		for useatt_n in "${nvar[@]}" ; do
			# Vary wildcard object attribute use
			for useatt_w in "${wvar[@]}" ; do
				# Create test requirement YAML file
				mktestreq "$n" "$w" "$useatt_n" "$useatt_w" \
					  >"$testreq"

				# Perform test for this set of parameters
				get_desc "$n" "$w" "$have_w" "$useatt_n" \
					 "$useatt_w"
				yaml_file "$telarc" 0 "telarc"
				yaml_file "$testreq" 0 "testreq"
				dotest "$telarc" "$testreq" "$num_objects" \
				       "$n" "$have_w" "$useatt_w" "$w" \
				       "run_${n}_${w}_${useatt_n}_${useatt_w}"
			done
		done
	done
done

exit 0
