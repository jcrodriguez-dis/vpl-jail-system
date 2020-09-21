#!/bin/bash
make all
if test -f "src/program-test" ; then
	./src/program-test
	if [ "$?" != "0" ] ; then
		tail /var/log/syslog
	else
		echo "Todas las pruebas pasadas."
	fi
else 
	echo "src/program-test not found."
fi

