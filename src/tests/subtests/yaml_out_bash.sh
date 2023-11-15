#!/bin/bash

source $TELA_BASH || exit 1

yaml "my: data"
yaml "my2: data2"
pass "pass"

# Report two testcases to catch residual YAML output.
pass "pass2"

exit $(exit_status)
