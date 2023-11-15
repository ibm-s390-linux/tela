# SPDX-License-Identifier: MIT
# common.bash - library of common Bash functions.
#
# Copyright IBM Corp. 2023
#
# This file contains Bash functions that are used in multiple libexec
# scripts. Use the source command to make use of these functions.

if [[ -n "$TELA_FRAMEWORK" ]] ; then
	TOOLNAME=${0##${TELA_FRAMEWORK}/}
else
	TOOLNAME=${0##*/}
fi
TELA_DEBUG=${TELA_DEBUG:=0}

#
# die - Emit error message and exit with non-zero exit code
#
# @msg: Error message
#
function die() {
	echo "$TOOLNAME: $*" >&2
	exit 1
}

#
# warn - Emit error message
#
# @msg: Error message
#
function warn() {
	echo "$TOOLNAME: $*" >&2
}

#
# debug - Emit debug message
#
# @msg: Debug message
#
if [[ "$TELA_DEBUG" != "0" ]] ; then
	if [[ -z "$_TELA_STARTTIME" ]] ; then
		# Initialize starttime (in ms) for global debug time stamps
		_TELA_STARTTIME=$(date +%s%N)
		export _TELA_STARTTIME=${_TELA_STARTTIME%??????}
	fi

	function debug() {
		local t=$(date +%s%N)
		t=${t%??????}
		(( t=t-$_TELA_STARTTIME ))
		printf "DEBUG: [%06sms] %6u: %s: %s: %s\n" \
			$t $$ "$TOOLNAME" "$SYSTEM" "$@" >&2
	}
else
	function debug() {
		:
	}
fi

#
# canonical_ccwdev_id - Convert CCW device ID to canonical format
#
# @var: Name of output variable
# @id: CCW device ID
#
# Convert the specified CCW device ID @id to canonical format:
#
#    <cssid>.<ssid>.<device_number>
#
# Store the result in the variable named @var. Return 0 if ID is valid,
# non-zero otherwise.
#

function canonical_ccwdev_id() {
	local _var=$1 _id=$2 _cssid _ssid _devno IFS

	IFS='.'
	set -- $_id
	unset IFS

	case $# in
	1) _cssid=0 ; _ssid=0 ; (( _devno=0x$1 )) ;;
	3) (( _cssid=0x$1 )) ; (( _ssid=0x$2 )) ; (( _devno=0x$3 )) ;;
	esac 2>/dev/null

	if [[ -z "$_cssid" || -z "$_ssid" || -z "$_devno" ]] ; then
		echo "$TOOLNAME: Invalid CCW device ID format: $_id" >&2
		return 1
	fi

	printf -v _id "%x.%x.%04x" $_cssid $_ssid $_devno

	eval "$_var=$_id"

	return 0
}

#
# canonical_chpid - Convert CHPID to canonical format
#
# @var: Name of output variable
# @id: CHPID
#
# Convert the specified CHPID @id to canonical format:
#
#    <cssid>.<chpid>
#
# Store the result in the variable named @var. Return 0 if ID is valid,
# non-zero otherwise.
#

function canonical_chpid() {
	local _var="$1" _id="$2" _cssid _chpid IFS

	IFS='.'
	set -- $_id
	unset IFS

	case $# in
	1) _cssid=0 ; (( _chpid=0x$1 )) ;;
	2) (( _cssid=0x$1 )) ; (( _chpid=0x$2 )) ;;
	esac 2>/dev/null

	if [[ -z "$_cssid" || -z "$_chpid" ]] ; then
		echo "$TOOLNAME: Invalid CHPID format: $_id" >&2
		return 1
	fi

	printf -v _id "%x.%02x" $_cssid $_chpid
	eval "$_var=$_id"

	return 0
}

#
# get_ccwdev_chpids - Get list of CHPIDs for specified CCW device
#
# @var: Name of output variable
# @id: CCW device ID
#
# Obtain the list of full IDs (<cssid>.<chpid> format) of each CHPID that is
# installed (PIM bit set) for the CCW device with the specified @id. Store
# the result as a space-separated list in the variable named @var.
#
function get_ccwdev_chpids() {
	local _var=$1 _id=$2 _list _sysfs _cssid _chpids _pim _pam _pom _pm
	local _chpid _im IFS

	_sysfs="/sys/bus/ccw/devices/$_id"

	[[ ! -e "$_sysfs" ]] && return

	_cssid=${_id%%.*}

	read _chpids 2>/dev/null <$_sysfs/../chpids
	read _pim _pam _pom 2>/dev/null <$_sysfs/../pimpampom
	(( _pim=0x$_pim ))
	(( _pm=0x100 ))

	# Generate an output line for each installed CHPID (PIM bit set)
	for _chpid in $_chpids ; do
		(( _pm=_pm>>1 ))
		(( _im=_pim&_pm ))
		[[ $_im -ne 0 ]] && _list="$_list $_cssid.$_chpid"
	done

	eval "$_var=\"$_list\""
}

#
# check_ccwdev_chpids - Create output for each CHPID of a CCW device
#
# @id: CCW device ID
# @indent: Indentation prefix
# @print_text: Text to print as value of attribute
# @print_id: Flag indicating whether to print ID
# @count_text: Optional count attribute name
#
# Create an output line for each CHPID defined for CCW device @id. The format
# is as follows:
#
#   <indent>chpid <chpid_id>:[ <print_text>][<chpid_id>]
#
# If @count_text is provided, print a final output line containing the number of
# CHPIDs found:
#
#   <indent><count_text>: <count>
#

function check_ccwdev_chpids() {
	local id="$1" indent="$2" print_text="$3" print_id="$4" count_text="$5"
	local line chpids chpid num=0 IFS

	[[ -z "$print_id" ]] && print_id=0

	get_ccwdev_chpids chpids $id

	for chpid in $chpids ; do
		line="${indent}chpid $chpid:"
		[[ -n "$print_text" ]] && line="${line} ${print_text}"
		[[ $print_id != 0 ]] &&	line="${line}${chpid}"

		echo "$line"
		(( num++ ))
	done
	[[ -n "$count_text" ]] && echo "${indent}${count_text}: $num"
}

#
# check_pnetids - Create output for each PNETID found in a utility string
#
# @util: Path to file containing utility string
# @indent: Indentation prefix
# @count_text: Optional count attribute name
#
# Create an output line for each PNETID found in a utility string. The format
# is as follows:
#
#   <indent>pnetid <num>:
#   <indent>  id: <pnetid>
#
# If @count_text is provided, print a final output line containing the number of
# CHPIDs found:
#
#   <indent><count_text>: <count>
#
function check_pnetids() {
	local util="$1" indent="$2" count_text="$3" id n num=0

	for n in 0 1 2 3 ; do
		id=$(dd if="$util" bs=16 count=1 skip=$n conv=ascii \
			2>/dev/null | tr -d '\0')
		if [[ -n "$id" && "$id" != "                " ]] ; then
			echo "${indent}pnetid $n:"
			echo "${indent}  id: \"$id\""
			(( num++ ))
		fi
	done
	[[ -n "$count_text" ]] && echo "${indent}${count_text}: $num"
}
