include ../tela.mak

CFLAGS += -fPIC

# Ensure that object files are rebuild on change to header files
headers := $(wildcard *.h)
objects := $(patsubst %.c,%.o,$(wildcard *.c))
$(objects): $(headers)

all: tela tela_api.o

tela: tela.o config.o misc.o log.o pretty.o record.o yaml.o resource.o console_zvm.o

clean:
	rm -f tela *.o
