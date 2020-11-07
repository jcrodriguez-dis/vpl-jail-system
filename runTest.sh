#!/bin/bash
./configure
make distcheck
if [ "$?" != "0" ] ; then
	echo "Error: make distcheck fails."
	exit 1
fi
cd tests
make program-test
if test -f program-test ; then
	rm -R cgroup.test 2> /dev/null
	cp -a cgroup cgroup.test
	./program-test
	if [ "$?" != "0" ] ; then
		tail /var/log/syslog
		exit 1
	else
		rm -R cgroup.test 2> /dev/null
		rm program-test
		echo "All tests passed."
		exit 0
	fi
else 
	echo "Error: program-test not found."
	exit 2
fi
