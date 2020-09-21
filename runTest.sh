#!/bin/bash
make all
if test -f "src/program-test" ; then
	./src/program-test
	if [ "$?" != "0" ] ; then
		tail /var/log/syslog
	else
		echo "All tests passed."
	fi
else 
	echo "src/program-test not found."
fi

