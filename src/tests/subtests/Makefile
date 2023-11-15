# Test programs in this directory are not run by tela, but by other tests for
# their output.

include ../../../tela.mak

incdir: incdir.c

incconflict: CFLAGS += -I./include/
incconflict: incconflict.c

null_fmt: null_fmt.c $(TELA_OBJ)

early_warn: early_warn.c $(TELA_OBJ)

yaml_out: yaml_out.c $(TELA_OBJ)

yaml_file: yaml_file.c $(TELA_OBJ)

report: report.c $(TELA_OBJ)

fail_all: fail_all.c $(TELA_OBJ)

skip_all: skip_all.c $(TELA_OBJ)

atresult: atresult.c $(TELA_OBJ)

clean:
	rm -f incdir incconflict null_fmt early_warn yaml_out report fail_all \
	      skip_all yaml_file atresult
