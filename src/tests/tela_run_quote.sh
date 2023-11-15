#!/bin/bash
#
# Check if tela correctly handles quoting of special characters in telarc
# files between tela match and tela run
#

source "$TELA_BASH" || exit 1

TELA="$TELA_FRAMEWORK/src/tela"

TESTYAML="tela_run_quote.testyaml"
TELARC="tela_run_quote.telarc"
EXEC="tela_run_quote_exec.sh"

# Match requirements
check_run_cmd -n "match" -o ".+" -e '' "$TELA" match "$TESTYAML" "$TELARC" 0
MATCHENV="$TELA_RUN_CMD_STDOUT"
read MATCHERR <"$TELA_RUN_CMD_STDERR"

# Check requirements
check_run_cmd -n "run" -o "SUCCESS" -e '' "$TELA" run "$EXEC" "" "$MATCHENV" "$MATCHERR"

exit $(exit_status)
