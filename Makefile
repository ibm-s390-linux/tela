include tela.mak

ifneq ($(wildcard tests/Makefile),)
  # Use actual testcase directory
  TESTS := tests
else
  # Demo mode
  TESTS := examples/api examples/match
endif

all:
	@$(MAKE) --silent -C src all

clean:
	@$(MAKE) --silent -C src clean
	@$(MAKE) --silent -C doc clean
