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
} &>/dev/null
./vpl_execution
exit
