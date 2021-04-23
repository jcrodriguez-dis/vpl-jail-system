#!/bin/bash
{
	export LC_ALL=$VPL_LANG 1>/dev/null 2>.vpl_set_locale_error
	#if current lang not available use en_US.utf8
	if [ -s .vpl_set_locale_error ] ; then
		rm .vpl_set_locale_error
		export LC_ALL=en_US.utf8
	fi
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
