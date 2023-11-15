#!/bin/bash
#
# Check if 'tela run' correctly handles an invalid .telarc file.
#

echo "Output:"

export TELA_RC=$(pwd)/inval.rc

# Filter through 'tela run'
$TELA_TOOL run /bin/true

exit $?
