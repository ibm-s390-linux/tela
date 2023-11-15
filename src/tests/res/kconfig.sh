#!/bin/bash

source $TELA_BASH || exit 1

# Skip test if config symbols cannot be accessed, e.g. due to chroot
if [[ ! -e "/boot/config-$(uname -r)" ]] && [[ ! -e "/proc/config.gz" ]] ; then
	skip_all "Cannot access kernel config"
fi

# Check if requirement succeeds
$TELA_TOOL run ./kconfig_ok >$TELA_TMP/out1 2>&2
yaml "output:"
yaml_file $TELA_TMP/out1 2
! grep -q SKIP $TELA_TMP/out1
ok $? kconfig_ok

$TELA_TOOL run ./kconfig_not_set >$TELA_TMP/out1 2>&2
yaml "output:"
yaml_file $TELA_TMP/out1 2
! grep -q SKIP $TELA_TMP/out1
ok $? kconfig_not_set

# Check if requirement fails
$TELA_TOOL run ./kconfig_not_ok >$TELA_TMP/out1 2>&2
yaml "output:"
yaml_file $TELA_TMP/out1 2
grep -q SKIP $TELA_TMP/out1
ok $? kconfig_not_ok

exit $(exit_status)
