include ../../../tela.mak

# Ensure deterministic results independent of test system's telarc
export TELA_RC := /dev/null

# os_id:      Check if TELA_OS_ID is available in Makefile
# os_version: Check if TELA_OS_VERSION is available in Makefile

TRUE := $(firstword $(wildcard $(addsuffix /true,$(subst :, ,$(PATH)))))
FALSE := $(firstword $(wildcard $(addsuffix /false,$(subst :, ,$(PATH)))))

TESTS := os_id os_version

os_id:
ifeq ($(TELA_OS_ID),)
	@cp $(FALSE) os_id
else
	@cp $(TRUE) os_id
endif

os_version:
ifeq ($(TELA_OS_VERSION),)
	@cp $(FALSE) os_version
else
	@cp $(TRUE) os_version
endif

clean:
	rm -f os_id os_version

.PHONY: os_id os_version
