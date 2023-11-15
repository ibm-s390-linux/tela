# Default recipes used to compile framework components and test programs
# when running as stand-alone repository

CFLAGS  := -g -W -Wall -Wformat-security -Wextra -Wno-unused-parameter \
	   -D_GNU_SOURCE
CROSS_COMPILE ?=

reldir = $(if $(TELA_BASE),$(subst $(TELA_BASE)/,,$(CURDIR)),$(CURDIR))
define cmd_define
        $(strip $(1))            = $(3)
        $(strip $(1))_SILENT    := $$($(strip $(1)))
        override $(strip $(1))   = $$(call echocmd,$(2),/$$@)$$($(strip $(1))_SILENT)
endef

$(eval $(call cmd_define,     AS,"  AS      ",$(CROSS_COMPILE)as))
$(eval $(call cmd_define,   LINK,"  LINK    ",$(CROSS_COMPILE)gcc))
$(eval $(call cmd_define,     LD,"  LD      ",$(CROSS_COMPILE)ld))
$(eval $(call cmd_define,     CC,"  CC      ",$(CROSS_COMPILE)gcc))
$(eval $(call cmd_define, HOSTCC,"  HOSTCC  ",gcc))
$(eval $(call cmd_define, LINKXX,"  LINKXX  ",$(CROSS_COMPILE)g++))
$(eval $(call cmd_define,    CXX,"  CXX     ",$(CROSS_COMPILE)g++))
$(eval $(call cmd_define,    CPP,"  CPP     ",$(CROSS_COMPILE)gcc -E))
$(eval $(call cmd_define,     AR,"  AR      ",$(CROSS_COMPILE)ar))
$(eval $(call cmd_define,     NM,"  NM      ",$(CROSS_COMPILE)nm))
$(eval $(call cmd_define,  STRIP,"  STRIP   ",$(CROSS_COMPILE)strip))
$(eval $(call cmd_define,OBJCOPY,"  OBJCOPY ",$(CROSS_COMPILE)objcopy))
$(eval $(call cmd_define,OBJDUMP,"  OBJDUMP ",$(CROSS_COMPILE)objdump))
$(eval $(call cmd_define,    CAT,"  CAT     ",cat))
$(eval $(call cmd_define,    SED,"  SED     ",sed))
$(eval $(call cmd_define,   GZIP,"  GZIP    ",gzip))
$(eval $(call cmd_define,     MV,"  MV      ",mv))
$(eval $(call cmd_define, PANDOC,"  PANDOC  ",pandoc))

ifeq ("${TELA_VERBOSE}", "2")
	echocmd=
else
	MAKEFLAGS += --quiet
	echocmd=echo $1$(call reldir)$2;
endif

all: all_check

help:
	@echo "Usage: $(MAKE) [TARGETS] [OPTIONS]"
	@echo
	@echo "TARGETS"
	@echo "  all        Build all testcases (default)"
	@echo "  check      Build and run all test cases"
	@echo "  clean      Delete all generated files"
	@echo "  telarc     Create .telarc template"
	@echo "  plan       Create test plan YAML from test.log"
	@echo "  telastate  Display resource state"
	@echo ""
	@echo "OPTIONS"
	@echo "  V=1|2           Show verbose test output (default: 0)"
	@echo "  PRETTY=0        Show TAP13 instead of formatted output (default: 1)"
	@echo "  TESTS=<name>    Only run tests with specified names"
	@echo "  LOG=<path>      Write TAP13 log to <path> (default: test.log)"
	@echo "  DATA=<path>     Write additional data to <path> (default: test.tgz)"
	@echo "  COLOR=0|1|auto  Control use of color in formatted output (default: auto)"
	@echo "  SCOPE=<value>   Control the test scope (default: quick)"
	@echo "  CACHE=0|1       Control caching of system state data (default: 0)"
	@echo "  BEFORE=<cmds>   Colon-separated list of commands to run before test start"
	@echo "  AFTER=<cmds>    Colon-separated list of commands to run after test end"
	@echo "  SKIPFILE=<file> Skip over tests listed or matched by shell pattern in <file>"
	@echo "  EXIT_FAIL=<int> Use exit code <int> for failure indication (default: 1)"
	@echo "  PCIFMT=fid|uid  Control PCI ID format to use for 'make telarc' (default: fid)"
	@echo "  RUNLOG=<path>   Write unprocessed test output to <path>"
	@echo "  RESFAIL=0|1     Control exit on missing .telarc resource (default: 0)"

clean_echo:
	$(call echocmd, "  CLEAN   ", "")

clean: clean_echo

.PHONY: all help clean_sub clean_echo clean_local clean
