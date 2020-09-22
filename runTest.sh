#!/bin/bash
make all
if test -f "src/program-test" ; then
	rm -R cgroup.test 2> /dev/null
	cp -a cgroup cgroup.test
	./src/program-test
	if [ "$?" != "0" ] ; then
		tail /var/log/syslog
		exit 1
	else		
		echo "All tests passed."
		exit 0
	fi
else 
	echo "src/program-test not found."
	exit 2
fi
