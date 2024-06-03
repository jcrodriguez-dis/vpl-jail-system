#!/bin/bash
# package:		Part of vpl-jail-system
# copyright:    Copyright (C) 20XX Juan Carlos Rodriguez-del-Pino
# license:      GNU/GPL, see LICENSE.txt or http://www.gnu.org/licenses/gpl-3.0.txt
# Description:  Script to run tests for vpl-jail-system

CHECK_MARK="✅"
X_MARK="❌"
function writeHeading {
	echo -e "\e[33m RUNNING \e[0m \e[34m$1\e[0m"
}
function writeInfo {
	echo -e $3 "\e[33m$1\e[0m$2"
}
function writeError {
	echo -e "\e[31m$1\e[0m$2"
}

function writeCorrect {
	echo -e "\e[32m$1\e[0m$2"
}

function write {
	echo -e "$1"
}

export -f writeHeading
export -f writeInfo
export -f writeError
export -f write

function runTests() {
	local test
	local fmessages=messages.txt
	local ferrors=errors.txt
	local ntests=$#
	local n=1
	local testresult
	if [ $ntests -gt 1 ] ; then
		writeInfo "" "$ntests tests"
	fi
	while [ "$1" != "" ] ; do
		test=$1
		if [ $ntests -gt 1 ] ; then
			writeInfo "Test $n" ": $test " -n
		fi
		{
			if [ $(type -t "$test") == "function" ] || [ -x "$test" ] ; then
				"$test"
			else
				echo "$test is not a function or command" > /dev/stderr
				"$test" &> /dev/null
			fi
		} 1>$fmessages 2>$ferrors
		testresult=$?
		if [ "$testresult" != "0" -a "$testresult" != "111" ] ; then
			writeError "Errors found" " $X_MARK"
			writeInfo "Standard error" " (max 100 lines)"
			tail -n 100 $ferrors
			writeInfo "Standard output" " (max 100 lines)"
			tail -n 100 $fmessages
			rm $fmessages 2> /dev/null
			rm $ferrors 2> /dev/null
			exit 1
		else
			writeCorrect "$CHECK_MARK"
			if [ "$testresult" == "111" ] ; then
				echo "$(cat $fmessages)"
			fi
		fi
		((n=n+1))
		shift
	done
	rm $fmessages 2> /dev/null
	rm $ferrors 2> /dev/null
	write
	if [ $ntests -gt 1 ] ; then
		writeCorrect "All tests passed $CHECK_MARK"
	fi
}

function Autotools_execution() {
	[ -x "$(command -v autoreconf)" ] && autoreconf -i
	aclocal
	autoheader
	autoconf
	automake
	./configure
}

function Packaging_for_distribution() {
	make distcheck
}

function Unit_tests() {
	local result
	cd tests
	make program-test 1>/dev/null
	if test -f program-test ; then
		rm -R cgroup.test 2> /dev/null
		cp -a cgroup cgroup.test
		rm -R files.test 2> /dev/null
		mkdir files.test
		mkdir files.test/a
		mkdir files.test/b
		mkdir files.test/a/b
		ln -s c files.test/a/l1
		ln -s ../a/b files.test/a/l2
		ln -s ../../b files.test/a/b/l3
		if [ -x "$(command -v valgrind)" ] ; then
			valgrind ./program-test 2> run.log
		else
			writeInfo "   " "Please, install valgrind for better checks"
			./program-test 2> run.log
		fi
		result=$?
		rm -R cgroup.test 2> /dev/null
		rm -R files.test 2> /dev/null
		rm program-test
		if [ "$result" != "0" -o "$SHOW_LOG" != "" ] ; then
			cat run.log
		fi
		rm run.log
		cd ..
		if [ "$result" != "0" ] ; then
			return 1
		else
			return 111
		fi
	fi
	cd ..
	return 1
}

function WebSocket_tests() {
	local result
	local running
	cd tests
	make websocket-echo-test > /dev/null
	if test -f websocket-echo-test ; then
		./websocket-echo-test &
		running=$!
		python3 ./webSocketClient.py
		result=$?
		kill $running
		cd ..
		if [ "$result" != "0" ] ; then
			tail /var/log/syslog
			return 1
		else
			return 111
		fi
	else
		echo "Compilation of websocket-echo-test failed"
	fi
	cd ..
	return 1
}

function Check_scripts() {
	local result=0
	local scripts
	local script
	scripts="runDockerComposeTest.sh runDockerTest.sh runTest.sh"
	scripts="$scripts uninstall-sh install-bash-sh install-vpl-sh vpl-jail-system.initd"
	for script in $scripts ; do
		bash -n $script
		[ "$?" != "0" ] && result=1
	done
	echo
	return $result
}

if [ -f ./config.h ] ; then
	VERSION=$(grep -E "PACKAGE_VERSION" ./config.h | sed -e "s/[^\"]\+\"\([^\"]\+\).*/\1/")
fi
if [ "$1" != "" ] ; then
	writeHeading "$1 of the vpl-jail-system $VERSION"
	runTests $1
else
	writeHeading "Tests of the vpl-jail-system $VERSION"
	runTests Autotools_execution Packaging_for_distribution Unit_tests WebSocket_tests Check_scripts
fi
