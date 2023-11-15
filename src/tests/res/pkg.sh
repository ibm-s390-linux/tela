#!/bin/bash

source $TELA_BASH || exit 1

# Check if requirement for installed package succeeds
$TELA_TOOL run ./pkg_ok >$TELA_TMP/out1 2>&2
yaml "output:"
yaml_file $TELA_TMP/out1 2
! grep -q SKIP $TELA_TMP/out1
ok $? pkg_ok

# Check if requirement for unknown package fails
$TELA_TOOL run ./pkg_not_ok >$TELA_TMP/out1 2>&2
yaml "output:"
yaml_file $TELA_TMP/out1 2
grep -q SKIP $TELA_TMP/out1
ok $? pkg_not_ok

exit $(exit_status)
