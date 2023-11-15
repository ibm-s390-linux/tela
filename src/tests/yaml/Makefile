include ../../../tela.mak

# Ensure deterministic results independent of test system's telarc
export TELA_RC := /dev/null

TESTS := warn.sh trav.sh trav2.sh

CFLAGS += -I ../..
FRAMEWORK_OBJS := $(addprefix $(TELASRC)/, yaml.o misc.o)

warn_files := $(wildcard warn_data/*.yaml)
warn_execs := $(patsubst %.yaml,%,$(warn_files))

warn.sh: warn.sh.yaml
warn.sh.yaml:
	printf "test:\n  plan: %d\n" $(words $(warn_files)) >warn.sh.yaml

trav_files := $(wildcard trav_data/*.yaml)

trav.sh: yamltest trav.sh.yaml
trav.sh.yaml:
	printf "test:\n  plan: %d\n" $(words $(trav_files)) >trav.sh.yaml

trav2_files := $(wildcard trav2_data/*.out)

trav2.sh: yamltest trav2.sh.yaml
trav2.sh.yaml:
	printf "test:\n  plan: %d\n" $(words $(trav2_files)) >trav2.sh.yaml

yamltest: yamltest.o $(FRAMEWORK_OBJS)
yamltest.o: yamltest.c

clean:
	rm -f warn.sh.yaml $(warn_execs) yamltest.o yamltest trav.sh.yaml \
	      trav2.sh.yaml
