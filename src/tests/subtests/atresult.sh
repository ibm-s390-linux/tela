#!/bin/bash

source $TELA_BASH || exit 1

name=""
result=""
args=""

function testcallback() {
	if [[ "$1" != "test" ]]; then
		exit 1
	fi

	if [[ "$2" != "pass" ]]; then
		exit 1
	fi

	if [[ "$3" != "arg1 arg2" ]]; then
		exit 1
	fi

	exit 0
}

atresult testcallback "arg1" "arg2"

pass "test"

# Should not happen
exit 1
