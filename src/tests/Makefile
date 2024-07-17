include ../../tela.mak

# Ensure deterministic results independent of test system's telarc
export TELA_NUMDOTS := -1
export TELA_RC      := /dev/null

TESTS := env.sh env2.sh incdir.sh incconflict.sh empty.sh null_fmt.sh
TESTS += early_warn.sh yaml_out_c.sh yaml_out_bash.sh yaml_file_c.sh
TESTS += yaml_file_bash.sh yaml/ report.sh build.sh log_file.sh
TESTS += fail_all.sh fail_all_bash.sh skip_all.sh skip_all_bash.sh
TESTS += tela_run_nl.sh tela_run_nonl.sh tela_run_null.sh tela_run_wrap.sh
TESTS += tela_run_fmt.sh tela_run_inval.sh tela_run_inval2.sh
TESTS += tela_run_timestamps.sh tela_run_pause_nl.sh tela_run_quote.sh
TESTS += tela_eval.sh tela_fixname.sh tela_match_sigpipe.sh
TESTS += testexec.sh stdin.sh res/ tela.mak/ tela/ atresult.sh
TESTS += skip_names/test.sh record_bash.sh record_get_bash.sh run_cmd.sh
TESTS += check_fd check_fd.sh check_run_cmd.sh unit/ wildcard/
TESTS += telarc_missing.sh

check_fd.sh: check_fd

check_fd: check_fd.c

clean:
	$(MAKE) -C subtests clean
	$(MAKE) -C build clean
	rm -f check_fd

.PHONY: check_fd.sh
