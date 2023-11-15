include ../../../tela.mak

# Ensure deterministic results independent of test system's telarc
export TELA_RC := /dev/null

TRUE := $(firstword $(wildcard $(addsuffix /true,$(subst :, ,$(PATH)))))

TESTS := match.sh pkg.sh kconfig.sh kmod.sh

pkg.sh: pkg_ok pkg_not_ok
kconfig.sh: kconfig_ok kconfig_not_ok kconfig_not_set
kmod.sh: kmod_ok kmod_not_ok

pkg_ok pkg_not_ok kconfig_ok kconfig_not_ok kmod_ok kmod_not_ok kconfig_not_set:
	cp $(TRUE) $@

clean:
	rm -f pkg_ok pkg_not_ok kconfig_ok kconfig_not_ok kmod_ok kmod_not_ok \
	      kconfig_not_set

.PHONY: pkg.sh
