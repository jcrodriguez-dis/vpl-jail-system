#!/bin/bash
echo -n 'vpl-jail' > /proc/$$/comm 2>/dev/null || :
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
	export TERM=dumb
	stty raw -echo iutf8 nl
} &>/dev/null
exec -a 'vpl-jail' ./vpl_execution
