#!/bin/bash

# getres(type, localid, [attr])
#
# Get attribute value for resource of type @type and local ID @localid.
# If @attr is specified, print attribute value, otherwise print resource ID.
function getres() {
	local type="$1" localid="$2" attr="${3:+_$3}"

	eval "echo \"\$TELA_SYSTEM_${type^^}_${localid}${attr^^}\""
}

# List characteristics of all assigned dummy resources
i=0
while [[ -n "$(getres dummy $i)" ]] ; do
	echo "Assigned dummy resource (local name '$i'):"
	echo "  ID ....: $(getres dummy $i)"
	echo "  SIZE ..: $(getres dummy $i size)"
	(( i++ ))
done

exit 0
