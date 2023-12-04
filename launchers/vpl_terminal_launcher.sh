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
} &>/dev/null
./vpl_execution
exit
