#!/bin/bash

source $TELA_BASH || exit 1

MK="./build_make.sh -C build/"
OUT=$TELA_TMP/out
ERR=$TELA_TMP/err
DIFF=$TELA_TMP/diff

function do_test() {
	local name cmd rc

	name=$1
	shift
	cmd="$*"

	yaml "cmd: make $cmd"
	$MK $cmd >$OUT 2>$ERR
	ok $? "$name"

	yaml "stdout: |"
	yaml_file $OUT 2
	diff -bu -I '^Result log stored' $OUT build/output_$name >$DIFF
	rc=$?
	yaml "stdout_diff: |"
	yaml_file $DIFF 2
	ok $rc "${name}_out"

	yaml "stderr: |"
	yaml_file $ERR 2
	[[ ! -s $ERR ]]
	ok $? "${name}_err"
}

# Ensure framework is built
$MK all >/dev/null 2>&1

# Check if 'make clean' works as expected
do_test clean clean

# Check if 'make all' works as expected
do_test all all

# Check if 'make check' works as expected
do_test check check

$MK clean >/dev/null 2>&1

# Check if 'make all' honors TESTS= variable
do_test all2 all TESTS=dir1/c

# Check if 'make check' honors TESTS= variable
do_test check2 check TESTS=a

# Check if 'make check' rebuilds tela tool if necessary
touch ${TELA_TOOL}.c
do_test rebuild check

exit $(exit_status)
