#!/bin/bash
{
	. vpl_environment.sh
	for NEWLANG in $VPL_LANG en_US.UTF-8 C.utf8 POSIX C
	do
		export LC_ALL=$NEWLANG 2> .vpl_set_locale_error
		if [ -s .vpl_set_locale_error ] ; then
			rm .vpl_set_locale_error
			continue
		else
			break
		fi
	done
	rm .vpl_set_locale_error
	export TERM=xterm
	stty sane iutf8 erase ^?
	# Calculate IP 127.X.X.X: (random port)
	if [ "$UID" == "" ] ; then
		echo "Error: UID not set"
	fi
	export serverPort=$((10000+$RANDOM%50000))
	export serverIP="127.$((1+$UID/1024%64)).$((1+$UID/16%64)).$((10+$UID%16))"
} &>/dev/null
./vpl_webexecution
exit
