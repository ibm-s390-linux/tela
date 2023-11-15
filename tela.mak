# NOTE: Including Makefiles should set 'TELA_DEFRECIPES=0' if they bring their
#       own set of default recipes for Makefile targets 'all' and 'clean'
TELA_DEFRECIPES ?= 1

ifeq ($(origin TESTS),environment)
  # Erase TESTS here to prevent a subdirectory Makefile without TESTS assignment
  # to inherit incorrect TESTS values from parent directory.
  TESTS :=
endif

# Default settings
V       := 0
PRETTY  := 1
SCOPE   := quick
LOG     := $(CURDIR)/test.log
DATA    := $(CURDIR)/test.tgz
CACHE   := 0
BEFORE  :=
AFTER   :=
SKIPFILE:=

TESTS        ?=
TEST_TARGETS ?=

# Paths
TELADIR  := $(abspath $(dir $(filter %tela.mak,$(MAKEFILE_LIST))))
TELASRC  := $(TELADIR)/src
LIBEXEC  := $(TELASRC)/libexec
TELAAPI  := $(TELASRC)/api
TELA_OBJ := $(addprefix $(TELASRC)/,tela_api.o config.o yaml.o misc.o)

export TELA_TOOL     ?= $(TELASRC)/tela
export TELA_REMOTE   ?= $(LIBEXEC)/remote
export TELA_BASH     ?= $(TELASRC)/tela.bash
export TELA_VERBOSE  ?= $(V)
export TELA_PRETTY   ?= $(PRETTY)
export TELA_SCOPE    ?= $(SCOPE)
export TELA_CACHE    ?= $(CACHE)
export TELA_BEFORE   ?= $(BEFORE)
export TELA_AFTER    ?= $(AFTER)
export TELA_LIB      ?= $(LIBEXEC)/lib

export TELA_WRITELOG  ?= $(LOG)
export TELA_WRITEDATA ?= $(DATA)
export TELA_RESFAIL   ?= $(RESFAIL)

# Log of unprocessed test program output intended for debugging purposes
ifneq ($(RUNLOG),)
  export TELA_RUNLOG    ?= $(abspath $(RUNLOG))
endif

# Framework base directory. Used to locate framework components.
export TELA_FRAMEWORK ?= $(TELADIR)

# Repository base directory. Used for Makefile echo commands.
export TELA_BASE ?= $(TELADIR)

# Testcase base directory. Test names are shown relative to this directory.
export TELA_TESTBASE ?= $(abspath $(CURDIR))

# Testsuite name
export TELA_TESTSUITE ?= $(notdir $(TELA_TESTBASE))

# Initialize starttime (in ms) for global debug time stamps
ifndef _TELA_STARTTIME
  export _TELA_STARTTIME := $(shell a=$$(date +%s%N) && echo $${a%??????})
endif

# Make path to filter file absolute if specified
ifneq ($(SKIPFILE),)
  ifeq ($(_TELA_SKIP),)
    export _TELA_SKIP=$(abspath $(SKIPFILE))
    ifeq ($(wildcard $(_TELA_SKIP)),)
      $(error SKIPFILE not found: $(SKIPFILE))
    endif
  endif
endif

# Ensure consistent tool output
export LANG   := C
export LC_ALL := C

# Additional environment
ifndef TELA_OS
  export TELA_OS := $(shell $(LIBEXEC)/os)
  export TELA_OS_ID := $(word 1,$(filter-out %:,$(TELA_OS)))
  export TELA_OS_VERSION := $(word 2,$(filter-out %:,$(TELA_OS)))
endif

# Establish default recipes unless provided by parent repository
ifneq ($(TELA_DEFRECIPES),0)
  include $(TELASRC)/default.mak
endif

# Must be present for all C test programs
CFLAGS += -I$(TELAAPI)

# Do not pass TESTS= specified on command line to subdirectories to allow
#   make TESTS=subdir
MAKEOVERRIDES :=

# Determine list of test subdirectories. Note: Deferred assignment required
# here to allow TESTS-assignment to occur after this Makefile
TELA_SUBDIRS = $(patsubst %/.,%,$(wildcard $(addsuffix /.,$(patsubst %/,%,$(TESTS)))))

# all is the default target
all: all_check

# Prevent duplicate 'make all' calls in sub-directories
ifndef _TELA_COMPILED
check: all_check
endif

check:
	@_TELA_COMPILED=1 $(LIBEXEC)/runtests.sh "$(MAKE)" $(TESTS)

count:
	@$(LIBEXEC)/counttests.sh "$(MAKE)" $(TESTS)

telarc:
	@$(LIBEXEC)/mktelarc.sh

plan:
	@$(LIBEXEC)/mkplan.sh

telastate:
	@$(LIBEXEC)/mkstate.sh


# SECONDEXPANSION + $$ required to allow 'include tela.mak'
# statement to appear at the beginning of a user's Makefile (when TESTS
# was not yet set).
.SECONDEXPANSION:

all_check:
ifndef _TELA_COMPILED
	@$(MAKE) -C $(TELASRC) $(notdir $(TELA_TOOL))
	@$(MAKE) -C $(TELASRC) $(notdir $(TELA_OBJ))
endif
	@$(MAKE) all_check2 _TELA_COMPILED=1 TESTS="$(TESTS)"

all_check2: $$(TESTS) $$(addsuffix .all,$$(TELA_SUBDIRS))

%.all:
	@$(MAKE) -C $(patsubst %.all,%,$@) all

clean_check: $$(addsuffix .clean,$$(TELA_SUBDIRS))
	@rm -f test.log test.tgz *.yaml.new

%.clean:
	@$(MAKE) -C $(patsubst %.clean,%,$@) clean

clean: clean_check

$(TELA_TOOL) $(TELA_OBJ):
	@$(MAKE) -C $(TELASRC) $(notdir $@)

# Provide an easy way to define targets that need a rebuild before 'make check'
all check: test_targets

test_targets:
	$(foreach target,$(TEST_TARGETS), \
                $(MAKE) -C $(dir $(target)) $(notdir $(target)) ; )

.PHONY: check all all_check all_check2 count clean clean_check test_targets telarc telastate
