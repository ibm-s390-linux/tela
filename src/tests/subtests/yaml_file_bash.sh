#!/bin/bash

source $TELA_BASH || exit 1

yaml_file "./yaml_text"
pass "text"

yaml_file "./yaml_text2" 2 "multiline"
pass "multiline"

yaml_file "./yaml_text3" 0 "hex" 1
pass "hex"

yaml_file "/dev/null" 0 "empty" 1
pass "empty"

# Report final testcase to catch residual YAML output
pass "residual"

exit $(exit_status)
