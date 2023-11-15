#!/bin/bash
#
# Check if framework-internal header files conflict with local header files.
#

make -C subtests incconflict || exit 1
exit 0
