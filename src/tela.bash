#
# SPDX-License-Identifier: MIT
# tela.bash - Bash implementation of tela API.
#
# Copyright IBM Corp. 2023
#

# Ensure that API is sourced only once
[[ "${_tela_bash_done:-0}" == "1" ]] && return

#
# pass - Report unconditional testcase success
#
# @name: Testcase name
#
function pass() {
	local name="$1"

	let _tap_testnum='_tap_testnum + 1'
	let _tap_num_pass='_tap_num_pass + 1'
	echo "ok     $_tap_testnum - $name" >&"$_tap_out"
	_after_test_cleanup "$name" "pass"
}

#
# fail - Report unconditional testcase failure
#
# @name: Testcase name
# @reason: Description why test was failed (optional)
#
function fail() {
	local name="$1" reason="${2-}"

	let _tap_testnum='_tap_testnum + 1'
	let _tap_num_fail='_tap_num_fail + 1'
	echo "not ok $_tap_testnum - $name" >&"$_tap_out"
	_after_test_cleanup "$name" "fail" "$reason"
}

#
# skip - Report that a testcase was skipped
#
# @name: Testcase name
# @reason: Description why test was skipped
#
function skip() {
	local name="$1" reason="$2"

	let _tap_testnum='_tap_testnum + 1'
	let _tap_num_skip='_tap_num_skip + 1'
	echo "ok     $_tap_testnum - $name # SKIP $reason" >&"$_tap_out"
	_after_test_cleanup "$name" "skip" "$reason"
}

#
# todo - Report that a testcase is not yet implemented
#
# @name: Testcase name
# @reason: Description what is missing
#
function todo() {
	local name="$1" reason="$2"

	let _tap_testnum='_tap_testnum + 1'
	let _tap_num_todo='_tap_num_todo + 1'
	echo "not ok $_tap_testnum - $name # TODO $reason" >&"$_tap_out"
	_after_test_cleanup "$name" "todo" "$reason"
}

#
# ok - Report testcase result depending on condition
#
# @cond: 0 for success, non-zero for failure
# @name: Testcase name
#
function ok() {
	local cond="$1" name="$2"

	if [ "$cond" -eq 0 ] ; then
		pass "$name"
		return 0
	else
		fail "$name"
		return 1
	fi
}

#
# fail_all - Report failure for all remaining planned testcases and exit
#
# @reason: Description why testcases were failed (optional)
#
function fail_all() {
	local reason="${1-}" i

	[ -z "$_tap_plan" ] && return

	for i in "${_tap_desc_keys[@]}"; do
		fail "$i" "$reason"
	done

	i=$_tap_testnum
	while [ "$i" -lt "$_tap_plan" ] ; do
		let i=$i+1
		fail "missing_name_$i" "$reason"
	done

	exit "$(exit_status)"
}

#
# skip_all - Report that all remaining planned testcases were skipped and exit
#
# @reason: Description why testcases were skipped
#
function skip_all() {
	local reason="$1" i

	[ -z "$_tap_plan" ] && return

	for i in "${_tap_desc_keys[@]}"; do
		skip "$i" "$reason"
	done

	i=$_tap_testnum
	while [ "$i" -lt "$_tap_plan" ] ; do
		let i='i + 1'
		skip "missing_name_$i" "$reason"
	done

	exit "$(exit_status)"
}

#
# bail - Abort test script execution
#
# @reason: Reason for aborting execution
#
function bail() {
	local reason="$*"

	echo "Bail out! $reason" >&"$_tap_out"
	exit 4
}

#
# yaml - Log structured data in YAML format
#
# @text: Arbitrary YAML data to write to result log
#
# For more details see doc/functions.md
#
function yaml() {
	local text="$*"

	echo "$text" >>"$_tela_yaml"
}

#
# yaml_file - Log YAML data from text file
#
# @filename: Name of file
# @indent: Level of indentation for file data
# @key: Optional mapping key to use
# @escape: Optional flag specifying whether to convert non-ASCII data
#
# For more details see doc/functions.md
#
function yaml_file() {
	local filename="$1" indent="$2" key="${3-}" escape="${4-}"

	[[ -z "$indent" ]] && indent=0
	[[ -z "$escape" ]] && escape=1

	if [[ -n "$key" ]] ; then
		printf "%*s%s:" $indent "" "$key" >>"$_tela_yaml"
		(( indent+=2 ))

		if [[ ! -s "$filename" ]] ; then
			# Use non-block scalar for empty files
			printf " \"\"\n" >>"$_tela_yaml"
			return
		fi

		printf " |2\n" >>"$_tela_yaml"
	fi

	$TELA_TOOL yamlscalar "$filename" "$indent" "$escape" >>"$_tela_yaml"
}

#
# diag - Log diagnostics data
#
# @text: Arbitrary text to write to result log as diagnostics data
#
function diag() {
	local text="$*"

	echo "# $text" >&"$_tap_out"
}

#
# exit_status - Retrieve combined exit status
#
# For more details see doc/functions.md
#
function exit_status() {
	local failed

	# Check for missing tests
	if [ -n "$_tap_plan" ] ; then
		[ "$_tap_testnum" -lt "$_tap_plan" ] && { echo 1 ; return ; }
	fi

	# Pure results
	[ "$_tap_num_pass" -eq "$_tap_testnum" ] && { echo 0 ; return ; }
	[ "$_tap_num_fail" -eq "$_tap_testnum" ] && { echo 1 ; return ; }
	[ "$_tap_num_skip" -eq "$_tap_testnum" ] && { echo 2 ; return ; }
	[ "$_tap_num_todo" -eq "$_tap_testnum" ] && { echo 3 ; return ; }

	# Mixed results
	let failed='_tap_num_fail + _tap_num_todo'

	[ "$failed" -gt 0 ] && { echo 1 ; return ; }

	echo 0
}

#
# fixname - Fix invalid characters in test name
#
# @testname: Original test name
#
# For more details see doc/functions.md
#
function fixname() {
	local testname="$1"

	$TELA_TOOL fixname "$testname"
}

#
# log_file - Log file as additional test result data
#
# @file: Path to file
# @name: Filename in log (optional)
#
# For more details see doc/functions.md
#
function log_file() {
	local file="$1"
	local name=""
	local log_file_sh="${TELA_FRAMEWORK:-$(readlink -f "$(dirname "${BASH_SOURCE[0]}")/..")}"/src/libexec/log_file.sh

	if [[ -z "$2" ]]; then
		name="${file##*/}"
	else
		name="$2"
	fi
	$log_file_sh "log_file" "$file" "$name"
}

#
# atresult - Register callback function for tests
#
# @callback: Function to register
# @data: Additional data to pass to callback (optional)
#
# For more details see doc/functions.md
#
function atresult() {
	_tela_atresult_cb="$1"
	_tela_atresult_data="${@:2}"
}

#
# push_cleanup - Register cleanup function to be called at test exit
#
# @callback: Function to be called at end of test
# @data: Additional data to pass as parameter to callback (optional)
#
# For more details see doc/functions.md
#
function push_cleanup() {
	local cb="$1" data="${2:-}"

	_tela_cleanup+=("$cb" "$data")
}

#
# pop_cleanup - Unregister most recently registered cleanup function(s)
#
# @num: Number of callbacks to unregister (optional, default is 1)
#
# For more details see doc/functions.md
#
function pop_cleanup() {
	local num=${1:-1}

	while [[ $(( num-- )) -gt 0 ]] ; do
		i="${#_tela_cleanup[@]}"

		[[ $i -eq 0 ]] && return

		unset _tela_cleanup[$(( i - 1 ))]
		unset _tela_cleanup[$(( i - 2 ))]
	done
}

function _do_cleanup() {
	local i cb data tmp="$_TELA_TMPDIR/cleanup"

	i=${#_tela_cleanup[@]}
	while [[ "$i" -gt 1 ]] ; do
		(( i-=2 ))
		cb="${_tela_cleanup[$i]}"
		data="${_tela_cleanup[$((i + 1))]}"

		"$cb" "$data" >"$tmp" 2>&1

		# Convert output to diagnostics data
		if [[ -s "$tmp" ]] ; then
			echo "# WARNING: $cb: Unexpected output during cleanup:"
			sed -e 's/^/# /g' <"$tmp"
		fi
	done
}

function _start_monitor() {
	local cmdline=()

	if [[ "$_tela_monitor_stdout" == 1 ]] ; then
		# Add stream for capturing stdout
		cmdline+=( "stdout:$_tela_monitor_fifo_stdout" )
	fi

	if [[ "$_tela_monitor_stderr" == 1 ]] ; then
		# Add stream for capturing stderr
		cmdline+=( "stderr:$_tela_monitor_fifo_stderr" )
	fi

	if [ "${#cmdline[@]}" -eq 0 ] ; then
		return
	fi

	# Need to ignore USR1 until tela monitor is setup
	trap "" SIGUSR1

	$TELA_TOOL monitor "${cmdline[@]}" >"$_tela_monitor_log" &
	_tela_monitor_pid=$!

	# Redirect stdout and stderr to monitored fifos
	if [[ "$_tela_monitor_stdout" == 1 ]] ; then
		exec >"$_tela_monitor_fifo_stdout"
	fi
	if [[ "$_tela_monitor_stderr" == 1 ]] ; then
		exec 2>"$_tela_monitor_fifo_stderr"
	fi
}

function _stop_monitor() {
	local do_log="${1:-1}"
	if [[ -z "$_tela_monitor_pid" ]] ; then
		return
	fi

	# Redirect stdout and stderr back to original FDs
	if [[ "$_tela_monitor_stdout" == 1 ]] ; then
		exec >&"${_tap_out}"
	fi
	if [[ "$_tela_monitor_stderr" == 1 ]] ; then
		exec 2>&"${_tap_err}"
	fi

	# Stop monitoring process
	kill -s USR1 "$_tela_monitor_pid" 2>/dev/null
	wait "$_tela_monitor_pid"
	_tela_monitor_pid=""

	[[  "$do_log" != 1 ]] && return

	# Process monitor output
	if [[ -s "$_tela_monitor_log" ]] ; then
		yaml "output: |"
		yaml_file "$_tela_monitor_log" 2
	else
		yaml 'output: ""'
	fi
}

#
# record - Record testcase program output
#
# @scope: Scope of recording specified as one of "all", "stdout", "stderr", or
#         "stop"
#
# Start recording all test case output with the specified scope:
# - all:       both standard output and standard error
# - stdout:    standard output
# - stderr:    standard error
# - stop:      stop recording
#
# Recorded output will automatically be added to the YAML-data of the next
# reported test result.
#
function record() {
	local scope="$1"

	# Stop previous monitor
	_stop_monitor

	if [[ ! -e "$_tela_monitor_fifo_stdout" ]] ; then
		# Create fifos
		mkfifo "$_tela_monitor_fifo_stdout"
		mkfifo "$_tela_monitor_fifo_stderr"
	fi

	# Initialize record log
	[[ "$scope" != "stop" ]] && echo -n > "$_tela_monitor_log"

	# Determine new monitor parameters
	_tela_monitor_stdout=0
	_tela_monitor_stderr=0

	case "$scope" in
	all)
		_tela_monitor_stdout=1
		_tela_monitor_stderr=1
		;;
	stdout)
		_tela_monitor_stdout=1
		;;
	stderr)
		_tela_monitor_stderr=1
		;;
	stop)
		;;
	esac

	# Start new monitor
	_start_monitor
}

#
# record_get - Retrieve recorded output data
#
# @scope: Scope of returned data - must be one of 'all', 'stdout', or 'stderr'
# @meta:  Flag that determines if log-meta-data is returned
#
# Print the log of output data that has most recently been recorded using the
# `record` command. If @meta is specified as 1, output is prefixed with
# timestamps and stream name.
#
function record_get() {
	local scope="$1" meta="$2"

	if [[ -z "$_tela_monitor_log" ]] ; then
		# No recorded data available
		return
	fi

	# Assume reasonable defaults if arguments are missing
	[[ -z "$scope" ]] && scope="all"
	[[ -z "$meta" ]]  && meta=1

	# Filter out lines according to requested scope
	case "$scope" in
	all)	cat
		;;
	stdout)	grep '^[^:]\+stdout[^:]*:'
		;;
	stderr)	grep '^[^:]\+stderr[^:]*:'
		;;
	esac <"$_tela_monitor_log" | {
		if [[ "$meta" == 1 ]] ; then
			# Pass-through
			cat
		else
			# Remove meta-data: [   0.000007] stdout(nonl): text
			sed -e 's/^[^:]\+: //g'
		fi
	}
}

#
# _after_test_cleanup - Executed after each test to do cleanup
#
function _after_test_cleanup() {
	local testname="$1" result="$2" reason="${3-}"

	# Stop monitor - this also logs recorded output as YAML testcase data
	_stop_monitor
	echo -n > "$_tela_monitor_log"

	_tap_print_yaml "$result" "$reason" "$testname"

	# Run callback
	if [[ -n "$_tela_atresult_cb" ]]; then
		$_tela_atresult_cb "$testname" "$result" "$_tela_atresult_data"
	fi

	# Process logged files
	_move_files "$1"

	# Restart monitor
	_start_monitor
}

#
# _move_files - Moves additional testdata from tmp_dir to the right path
#
function _move_files() {
	local log_file_sh="${TELA_FRAMEWORK:-$(readlink -f "$(dirname "${BASH_SOURCE[0]}")/..")}"/src/libexec/log_file.sh

	$log_file_sh "move_files" "$1"
}


#
# _tap_print_yaml - Print additional test case data in YAML format
#
function _tap_print_yaml() {
	local result="$1" reason="$2" testname="$3"
	local now duration dsec dmsec line last_str now_str i

	now=$(date +%s.%N)
	# Add formatted versions of start and stop times
	last_str=" # $(date --date="@$_tap_last" +"%F %T%z")"
	now_str=" # $(date --date="@$now" +"%F %T%z")"

	if [ -n "$reason" ] ; then
		reason=$(printf "\n  reason: \"%s\"" "$reason")
	fi

	let duration="(${now%.*} - ${_tap_last%.*}) * 1000000000"
	# Prefix 1 to prevent octal interpretation
	let duration="duration +1${now#*.} - 1${_tap_last#*.}"
	# Add 500 to achieve correct rounding
	let duration='(duration + 500) / 1000'
	let dsec='duration / 1000'
	let dmsec='duration % 1000'
	duration=$(printf "%d.%03d" "$dsec" "$dmsec")

	echo "  ---" >&"$_tap_out"
	sed -e 's/^/  /g' "$_tela_yaml" >&"$_tap_out"
	echo -n > "$_tela_yaml"
	for i in "${!_tap_desc_keys[@]}"; do
		if [[ "${_tap_desc_keys[$i]}" = "${testname}" ]]; then
			unset "_tap_desc_keys[$i]"
			break
		fi
	done
	[[ -v _tap_desc["$testname"] ]] && echo "  desc: \"${_tap_desc[$testname]}\"" >&"$_tap_out"
	cat >&"$_tap_out" <<EOF
  testresult: "$1"$reason
  testexec: "$_tap_exec"
  starttime: $_tap_last$last_str
  stoptime:  $now$now_str
  duration_ms: $duration
  ...
EOF
	_tap_last=$(date +%s.%N)
}

#
# __close_fds - Close all given file descriptors
#
function __close_fds() {
	local fds=("$@")

	for fd in "${fds[@]}"; do
		exec {fd}>&-
	done
}

#
# _replace_newlines_by_literals - Replace newlines by '\n' literal
#
function _replace_newlines_by_literals() {
	local out='' IFS=$'\n' line

	if [ "$#" -eq 1 ]; then
		out="${1//$'\n'/\\n}"
	else
		while read -r line; do
			out="$out$line\\n"
		done
	fi

	printf '%s' "$out"
}

#
# _grep - Print reason if content doesn't match the pattern
#
# @__output: Variable name to store the result
# @__pattern: Perl-compatible regular expression (PCRE) to match
# @__file: Search for pattern @__pattern in file @__file
# @__stream (optional): stream name, defaults to 'stdout'
#
# There are two cases that are different to the normal grep behavior:
# 1. if @__pattern is the empty string then @file must be empty
# 2. if @__pattern == '.*' @__file is allowed to be empty
function _grep() {
	local __output="$1" __pattern="$2" __file="$3" __stream="${4-stdout}"
	local __reason=''

	if [ ! -e "$__file" ]; then
		echo "File '$__file' doesn't exist" >&2
		return
	fi

	# '.*' matches against everything
	if [[ "$__pattern" == '.*' ]]; then
		return
	fi

	if [ -z "$__pattern" ]; then
		[ ! -s "$__file" ] || __reason="Output on $__stream not as expected"
	else
		__pattern="$(_replace_newlines_by_literals "$__pattern" 0<&-)"
		grep -q -Pz -- "$__pattern" "$__file" || __reason="Output on $__stream not as expected"
	fi
	printf -v "$__output" '%s' "$__reason"
}

function __set_run_cmd_path() {
	local __output="$1" __name="$2" __type="${3:-stdout}"

	printf -v "$__output" '%s' "${TELA_TMP}/${__name}_${__type}"
	printf -v "TELA_RUN_CMD_${__type^^}" '%s' "${!__output}"
}

function __run_cmd_yaml_report() {
	local stream_type="$1" file="$2" expected="$3"
	local exp_file

	yaml "${stream_type}:"
	if [[ -z "$expected" ]] ; then
		yaml "  expect: 'No output'"
	elif [[ "$expected" == '.*' ]]; then
		yaml "  expect: 'Do not care'"
	else
		exp_file="$_TELA_TMPDIR/run_cmd.expected"
		echo "$expected" > "$exp_file"
		yaml_file "$exp_file" 2 "expect"
		rm -f -- "${exp_file}"
	fi

	yaml_file "$file" 2 'actual'
}

function __assert_run_cmd_results() {
	local testname=$1 cmd_file=$2 status=$3 expected_status=$4 stdout_path=$5
	local expected_stdout=$6 stderr_path=$7 expected_stderr=$8 reason=''

	if [[ "$status" != "$expected_status" ]]; then
		reason="Exit code not as expected"
	fi

	if [ -z "$reason" ]; then
		_grep 'reason' "$expected_stdout" "$stdout" 'stdout'
	fi
	if [ -z "$reason" ]; then
		_grep 'reason' "$expected_stderr" "$stderr" 'stderr'
	fi

	[ -n "$reason" ] && yaml "reason: '$reason'"
	yaml_file "${cmd_file}" 0 'cmd'
	yaml "rc:"
	yaml "  expect: $expected_status"
	yaml "  actual: $status"

	__run_cmd_yaml_report 'stdout' "$stdout_path" "$expected_stdout"
	__run_cmd_yaml_report 'stderr' "$stderr_path" "$expected_stderr"

	[ -z "$reason" ]
	ok $? "$(fixname "$testname")"
}

#
# check_run_cmd - Check if the executed command reports values as expected
#
# check_run_cmd -n <name> [-s <value>] [-o <regex>] [-e <regex>] [-f <file>] [-g <file>] <command...>
#
# Run command <command...> and report success for test <name> if the
# exit code and output on stdout and stderr look as expected. Input for
# the command can be provided by providing STDIN, e.g. via pipe (e.g.
# `echo test |check_run_cmd ...`).
#
# After this function returns, the tested command's output will be accessible
# via environment variables `${TELA_RUN_CMD_STDOUT}` and
# `${TELA_RUN_CMD_STDERR}`.
#
# <command...> specifies the command to be executed.
#
# There are several options:
# -n <TESTNAME>
#    The test name
# -s <value> (optional)
#    Expected status/exit code <value> of the executed command, defaults to 0
# -o <regex> (optional)
#    Perl-compatible regular expression of what to expect on stdout, defaults to '.*'
# -e <regex> (optional)
#    Perl-compatible regular expression of what to expect on stderr, defaults to '.*'
# -f <file> (optional)
#    stdout path
# -g <file> (optional)
#    stderr path
#
function check_run_cmd() {
	local testname=
	local -a cmd
	local expected_status=0 expected_stdout='.*' expected_stderr='.*'
	local stdout= stderr= status reason=''
	local optstring='n:s:o:e:f:g:' OPTIND OPTION OPTARG

	while getopts "$optstring" 'OPTION'; do
		case "$OPTION" in
		n)
			testname="${OPTARG}"
			;;
		s)
			expected_status="${OPTARG}"
			;;
		o)
			expected_stdout="${OPTARG}"
			;;
		e)
			expected_stderr="${OPTARG}"
			;;
		f)
			stdout="${OPTARG}"
			;;
		g)
			stderr="${OPTARG}"
			;;
		*)
			return 1;
			;;
		esac
	done
	# The test name and the command is required. Everything else has default
	# values.
	if [ -z "$testname" ]; then
		echo 'Test name is missing' 2>&1
		return 1
	fi

	shift "$((OPTIND-1))"
	cmd=( "$@" )

	if [ -z "$stdout" ]; then
		__set_run_cmd_path 'stdout' "$testname"
	else
		TELA_RUN_CMD_STDOUT="$stdout"
	fi

	if [ -z "$stderr" ]; then
		__set_run_cmd_path 'stderr' "$testname" 'stderr'
	else
		TELA_RUN_CMD_STDERR="$stderr"
	fi

	cmd_file="${_TELA_TMPDIR}/check_run_cmd"
	(__close_fds "${_tela_close_fds[@]}" &>/dev/null; unset PS4; set -x; "${cmd[@]}" >"$stdout" 2>"$stderr") 2> "${cmd_file}"
	status=$?

	__assert_run_cmd_results "$testname" "${cmd_file}" "$status" \
		 "$expected_status" "$stdout" "$expected_stdout" \
		 "$stderr" "$expected_stderr"
}

#
# run_cmd - Check if command results in normal output
#
# @cmd: Command line to run
# @name: Test name
# @expect_rc: Expected exit code
# @expect_stdout: Flag indicating whether output on stdout is expected
# @expect_stderr: Flag indicating whether output on stderr is expected
#
# Run command line @cmd and report success for test @name if the command
# exit code and output on stdout and stderr match expected values.
#
# @expect_rc defines the expected exit code. Non-zero @expect_stdout and
# @expect_stderr values indicate that output is expected on stdout and stderr
# respectively. If a value of 2 is provided it is ignored if there is an actual
# output on stderr/stdout or not but it is still printed.
#
# After this function returns, the tested command's output will be accessible
# via environment variables `${TELA_RUN_CMD_STDOUT}` and
# `${TELA_RUN_CMD_STDERR}`.
#
function run_cmd() {
	local cmd="$1" name="$2" expect_rc="$3" expect_stdout="$4" expect_stderr="$5"
	local stdout stderr rc expected_stdout='' expected_stderr=''

	__set_run_cmd_path 'stdout' "$name"
	__set_run_cmd_path 'stderr' "$name" 'stderr'

	# there must be output on stdout/stderr
	[[ "$expect_stdout" == 1 ]] && expected_stdout='.+'
	[[ "$expect_stderr" == 1 ]] && expected_stderr='.+'

	# ignore stdout/stderr => accept everything
	[[ "$expect_stdout" == 2 ]] && expected_stdout='.*'
	[[ "$expect_stderr" == 2 ]] && expected_stderr='.*'

	cmd_file="${_TELA_TMPDIR}/run_cmd"
	(__close_fds "${_tela_close_fds[@]}"; unset PS4; eval "set -x; $cmd >'$stdout' 2>'$stderr'") >/dev/null 2> "${cmd_file}"
	rc=$?

	__assert_run_cmd_results "$name" "${cmd_file}" "$rc" "$expect_rc" "$stdout" \
		 "$expected_stdout" "$stderr" "$expected_stderr"
}

#
# _tela_exit - Internal cleanup function
#
function _tela_exit() {
	local rc=$?

	_do_cleanup

	# Handle test exit without any reported test results
	if [[ "$_tap_testnum" -eq 0 ]] ; then
		fail_all "Test exit with rc=$rc without reporting results!"
	fi

	_stop_monitor 0

	# Log unhandled record data as diagnostics information
	if [[ -s "$_tela_monitor_log" ]] ; then
		echo "# WARNING: Unhandled record data at test exit:"
		sed -e 's/^/# /g' <"$_tela_monitor_log"
	fi >&"$_tap_out"
}

#
# _tap_init - Internal initialization function
#
function _tap_init() {
	_tap_out=""
	_tap_err=""
	_tap_plan=""
	_tap_testnum=0
	_tap_num_pass=0
	_tap_num_fail=0
	_tap_num_skip=0
	_tap_num_todo=0
	_tap_tool="$TELA_TOOL"
	_tap_last=""
	_tap_exec="${TELA_EXEC:-$(readlink -f "${BASH_SOURCE[0]}")}"
	_tela_monitor_pid=""
	_tela_monitor_fifo_stdout=""
	_tela_monitor_fifo_stderr=""
	_tela_monitor_stdout=0
	_tela_monitor_stderr=0
	_tela_yaml="$_TELA_TMPDIR/yaml"
	_tela_monitor_fifo_stdout="$_TELA_TMPDIR/monitor.stdout"
	_tela_monitor_fifo_stderr="$_TELA_TMPDIR/monitor.stderr"
	_tela_monitor_log="$_TELA_TMPDIR/monitor.log"
	_tela_atresult_cb=
	_tela_cleanup=()
	_tela_close_fds=()
	_tela_bash_done=0
	declare -A -g _tap_desc
	# _tap_desc_keys is used to remember the correct order of the keys
	declare -a -g _tap_desc_keys
	local line IFS key YAMLPATH VALUE TYPE

	if [ -z "$TELA_TOOL" ] ; then
		echo "Error: Use 'make check' to run this test" >&2
		exit 4
	fi
	if [ ! -x "$TELA_TOOL" ] ; then
		echo "Error: Missing tela tool" >&2
		exit 4
	fi

	# Duplicate original stdout and stderr for use by this script
	if ! exec {_tap_out}>&1; then
		echo "Error: Could not duplicate stdout" >&2
		exit 4
	fi
	_tela_close_fds+=( "$_tap_out" )
	if ! exec {_tap_err}>&2; then
		echo "Error: Could not duplicate stderr" >&2
		exit 4
	fi
	_tela_close_fds+=( "$_tap_err" )

	echo "TAP version 13" >&"$_tap_out"

	_tap_plan=$($_tap_tool count "$_tap_exec")
	if [ -n "$_tap_plan" ] ; then
		echo "1..$_tap_plan" >&"$_tap_out"
	fi

	if [ -f "${_tap_exec}.yaml" ]; then
		while read -r line ; do
			eval "$line"
			if [[ "$TYPE" == "map" ]]; then
				key="${YAMLPATH#test/plan/}"
				_tap_desc["$key"]="$VALUE"
				_tap_desc_keys+=("$key")
			fi
		done < <($_tap_tool yamlget "$_tap_exec.yaml" "test/plan/*")

		while read -r line ; do
			eval "$line"
			key="${YAMLPATH#test/plan/}"
			key="${key%%/}"
			VALUE="${VALUE//\\/\\\\}"
			VALUE="${VALUE//\"/\\\"}"
			_tap_desc["$key"]="$VALUE"
		done < <($_tap_tool yamlget "$_tap_exec.yaml" "test/plan/*/")
	fi

	# Initialize YAML log file
	echo -n >"$_tela_yaml"

	# Note starting timestamp to get duration of first test
	_tap_last=$(date +%s.%N)

	# Queue cleanup function
	trap _tela_exit EXIT

	_tela_bash_done=1
}

_tap_init
