#!/bin/bash
#
# Check if tela.mak correctly provides include directory.
#

make -C subtests incdir || exit 1
exit 0
