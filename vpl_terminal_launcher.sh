#!/bin/bash
export LC_ALL=$VPL_LANG 1>/dev/null 2>.vpl_set_locale_error
#if current lang not available use en_US.utf8
if [ -s .vpl_set_locale_error ] ; then
	export LC_ALL=en_US.utf8 1>/dev/null 2> /dev/null
fi
rm .vpl_set_locale_error 1>/dev/null 2> /dev/null
stty sane iutf8 erase ^? 1>/dev/null 2>/dev/null
./vpl_execution
exit
