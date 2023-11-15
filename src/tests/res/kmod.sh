#!/bin/bash

source $TELA_BASH || exit 1

# Skip test if module data cannot be accessed, e.g. due to chroot
if [[ ! -d "/lib/modules/$(uname -r)/" ]] ; then
	skip_all "Cannot access kernel modules"
fi
# Check if requirement succeeds
$TELA_TOOL run ./kmod_ok >$TELA_TMP/out1 2>&2
yaml "output:"
yaml_file $TELA_TMP/out1 2
! grep -q SKIP $TELA_TMP/out1
ok $? kmod_ok

# Check if requirement fails
$TELA_TOOL run ./kmod_not_ok >$TELA_TMP/out1 2>&2
yaml "output:"
yaml_file $TELA_TMP/out1 2
grep -q SKIP $TELA_TMP/out1
ok $? kmod_not_ok

exit $(exit_status)
