#!/bin/bash

source $TELA_BASH || exit 1

fail_all "reason"

# Should not happen
fail "exit"

exit $(exit_status)
