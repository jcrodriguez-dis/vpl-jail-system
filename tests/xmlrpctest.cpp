/**
 * package:		Part of vpl-jail-system
 * copyright:	Copyright (C) 2022 Juan Carlos Rodríguez-del-Pino. All rights reserved.
 * license:		GNU/GPL, see LICENSE.txt or http://www.gnu.org/licenses/gpl-3.0.html
 **/

#include "basetest.h"
#include <cassert>
#include "../src/util.h"
#include "../src/xmlrpc.h"

class XMLRPCTest: public BaseTest {
	void testAvailable() {
		string rawdata = Util::readFile("./xmlrpcdata/available.xml");
		XMLRPC rpc(rawdata);
		string method = rpc.getMethodName();
		assert(method == "available");
		mapstruct data = rpc.getData();
		assert(data["maxmemory"]->getLong() == 10240);
	}

	void innerTest(string file, string method, string adminticket, long long pluginversion) {
		string rawdata = Util::readFile("./xmlrpcdata/" + file);
		XMLRPC rpc(rawdata);
		assert(rpc.getMethodName() == method);
		mapstruct data = rpc.getData();
		assert(data["adminticket"]->getString() == adminticket);
		assert(data["pluginversion"]->getLong() == pluginversion);
	}

	void testGetresult() {
		innerTest("getresult.xml", "getresult", "751514816568718", 2021052513L);
	}

	void testStop() {
		innerTest("stop.xml", "stop", "133355567641662", 2021052513L);
	}

	void testRunning() {
		innerTest("running.xml", "running", "751514816568718", 2021052513L);
	}

	void testRequest() {
		string rawdata = Util::readFile("./xmlrpcdata/request.xml");
		XMLRPC rpc(rawdata);
		string method = rpc.getMethodName();
		assert(method == "request");
		mapstruct data = rpc.getData();
		assert(data["maxtime"]->getLong() == 240);
		assert(data["maxfilesize"]->getLong() == 67108864);
		assert(data["maxmemory"]->getLong() == -2147483648);
		assert(data["maxprocesses"]->getLong() == 500);
		assert(data["runscript"]->getString() == "");
		assert(data["debugscript"]->getString() == "");
		assert(data["userid"]->getString() == "3");
		assert(data["activityid"]->getString() == "4382");
		assert(data["execute"]->getString() == "vpl_run.sh");
		assert(data["interactive"]->getLong() == 1);
		assert(data["lang"]->getString() == "en_US.UTF-8");
		assert(data["pluginversion"]->getLong() == 2021052513L);

		mapstruct files = rpc.getFiles();
		string filecontent;
		filecontent = "#!/bin/bash&#10;# This file is part of VPL for Moodle - http://vpl.dis.ulpgc.es/&#10;# Script for running C++ language&#10;# Copyright (C) 2012 Juan Carlos Rodr&#195;&#173;guez-del-Pino&#10;# License http://www.gnu.org/copyleft/gpl.html GNU GPL v3 or later&#10;# Author Juan Carlos Rodr&#195;&#173;guez-del-Pino &#60;jcrodriguez@dis.ulpgc.es&#62;&#10;&#10;#@vpl_script_description Using default g++ with math and util libs&#10;#load common script and check programs&#10;. common_script.sh&#10;check_program g++&#10;if [ &#34;$1&#34; == &#34;version&#34; ] ; then&#10;&#9;echo &#34;#!/bin/bash&#34; &#62; vpl_execution&#10;&#9;echo &#34;g++ --version | head -n2&#34; &#62;&#62; vpl_execution&#10;&#9;chmod +x vpl_execution&#10;&#9;exit&#10;fi &#10;get_source_files cpp C&#10;# Generate file with source files&#10;generate_file_of_files .vpl_source_files&#10;# Compile&#10;g++ -fno-diagnostics-color -o vpl_execution $2 @.vpl_source_files -lm -lutil&#10;rm .vpl_source_files&#10;";
		filecontent = XML::decodeXML(filecontent);
		assert(files["vpl_run.sh"]->getString() == filecontent);
		filecontent = "#include &#60;iostream&#62;&#10;using namespace std;&#10;int main() {&#10;    int m = 1000 * 1024;&#10;    int s = 0;&#10;    char* v;&#10;    while ((v = new char[m]) != NULL) {&#10;        for (int i = 0; i &#60; m; i++)&#10;            v[i] = (char)i;&#10;        s++;&#10;        cout &#60;&#60; s &#60;&#60; &#34;Mb&#34; &#60;&#60; endl;&#10;    }&#10;}&#10;";
		filecontent = XML::decodeXML(filecontent);
		assert(files["a.cpp"]->getString() == filecontent);
		filecontent = "#!/bin/bash&#10;export VPL_LANG='en_US.UTF-8'&#10;export VPL_SUBFILE0='a.cpp'&#10;export VPL_SUBFILES=&#34;a.cpp&#10;&#34;&#10;export VPL_VARIATION=''&#10;&#10;export VPL_XGEOMETRY='1010x698'&#10;";
		filecontent = XML::decodeXML(filecontent);
		assert(files["vpl_environment.sh"]->getString() == filecontent);
		filecontent = "#!/bin/bash&#10;# Default common funtions for scripts of VPL&#10;# Copyright (C) 2016 Juan Carlos Rodr&#195;&#173;guez-del-Pino&#10;# License http://www.gnu.org/copyleft/gpl.html GNU GPL v3 or later&#10;# Author Juan Carlos Rodr&#195;&#173;guez-del-Pino &#60;jcrodriguez@dis.ulpgc.es&#62;&#10;&#10;#load VPL environment vars&#10;if [ &#34;$PROFILE_RUNNED&#34; == &#34;&#34; ] ; then&#10;&#9;export PROFILE_RUNNED=yes&#10;&#9;if [ -f /etc/profile ] ; then&#10;&#9;&#9;cp /etc/profile .localvplprofile&#10;&#9;&#9;chmod +x .localvplprofile&#10;&#9;&#9;. .localvplprofile&#10;&#9;&#9;rm .localvplprofile&#10;&#9;fi&#10;fi&#10;. vpl_environment.sh&#10;#Use current lang&#10;export LC_ALL=$VPL_LANG 1&#62;/dev/null 2&#62;vpl_set_locale_error&#10;#If current lang not available use en_US.UTF-8&#10;if [ -s vpl_set_locale_error ] ; then&#10;&#9;export LC_ALL=en_US.UTF-8  1&#62;/dev/null 2&#62;/dev/null&#10;fi&#10;rm vpl_set_locale_error 1&#62;/dev/null 2&#62;/dev/null&#10;#functions&#10;&#10;# Wait until a program ($1 e.g. execution_int) of the current user ends. &#10;function wait_end {&#10;&#9;local PSRESFILE&#10;&#9;PSRESFILE=.vpl_temp_search_program&#10;&#9;#wait start until 5s&#10;&#9;for I in 1 .. 5&#10;&#9;do&#10;&#9;&#9;sleep 1s&#10;&#9;&#9;ps -f -u $USER &#62; $PSRESFILE&#10;&#9;&#9;grep $1 $PSRESFILE &#38;&#62; /dev/null&#10;&#9;&#9;if [ &#34;$?&#34; == &#34;0&#34; ] ; then&#10;&#9;&#9;&#9;break&#10;&#9;&#9;fi&#10;&#9;done&#10;&#9;while :&#10;&#9;do&#10;&#9;&#9;sleep 1s&#10;&#9;&#9;ps -f -u $USER &#62; $PSRESFILE&#10;&#9;&#9;grep $1 $PSRESFILE &#38;&#62; /dev/null&#10;&#9;&#9;if [ &#34;$?&#34; != &#34;0&#34; ] ; then&#10;&#9;&#9;&#9;rm $PSRESFILE&#10;&#9;&#9;&#9;return&#10;&#9;&#9;fi&#10;&#9;done&#10;}&#10;&#10;# Adds code to vpl_execution for getting the version of $PROGRAM&#10;# $1: version command line switch (e.g. -version)&#10;# $2: number of lines to show. Default 2&#10;function get_program_version {&#10;&#9;local nhl&#10;&#9;if [ &#34;$2&#34; == &#34;&#34; ] ; then&#10;&#9;&#9;nhl=2&#10;&#9;else&#10;&#9;&#9;nhl=$2&#10;&#9;fi&#10;&#9;&#10;&#9;echo &#34;#!/bin/bash&#34; &#62; vpl_execution&#10;&#9;if [ &#34;$1&#34; == &#34;unknown&#34; ] ; then&#10;&#9;&#9;echo &#34;echo \\&#34;$PROGRAM version unknown\\&#34;&#34; &#62;&#62; vpl_execution&#10;&#9;else&#10;&#9;&#9;echo &#34;$PROGRAM $1 | head -n $nhl&#34; &#62;&#62; vpl_execution&#10;&#9;fi&#10;&#9;chmod +x vpl_execution&#10;&#9;exit&#10;}&#10;&#10;# Populate SOURCE_FILES, SOURCE_FILES_LINE and SOURCE_FILE0 with files&#10;# of extensions passed. E.g. get_source_files cpp C&#10;function get_source_files {&#10;&#9;local ext&#10;&#9;SOURCE_FILES=&#34;&#34;&#10;&#9;SOURCE_FILES_LINE=&#34;&#34;&#10;&#9;for ext in &#34;$@&#34;&#10;&#9;do&#10;&#9;&#9;if [ &#34;$ext&#34; == &#34;NOERROR&#34; ] ; then&#10;&#9;&#9;&#9;break&#10;&#9;&#9;fi&#10;&#9;    local source_files_ext=&#34;$(find . -name &#34;*.$ext&#34; -print | sed 's/^.\\///g' | sed 's/ /\\\\ /g')&#34;&#10;&#9;    if [ &#34;$SOURCE_FILES_LINE&#34; == &#34;&#34; ] ; then&#10;&#9;        SOURCE_FILES_LINE=&#34;$source_files_ext&#34;&#10;&#9;    else&#10;&#9;        SOURCE_FILES_LINE=$(echo -en &#34;$SOURCE_FILES_LINE\\n$source_files_ext&#34;)&#10;&#9;    fi&#10;&#9;    local source_files_ext_s=&#34;$(find . -name &#34;*.$ext&#34; -print | sed 's/^.\\///g')&#34;&#10;&#9;    if [ &#34;$SOURCE_FILES&#34; == &#34;&#34; ] ; then&#10;&#9;        SOURCE_FILES=&#34;$source_files_ext_s&#34;&#10;&#9;    else&#10;&#9;        SOURCE_FILES=$(echo -en &#34;$SOURCE_FILES\\n$source_files_ext_s&#34;)&#10;&#9;    fi&#10;&#9;done&#10;&#10;    if [ &#34;$SOURCE_FILES&#34; != &#34;&#34; -o &#34;$1&#34; == &#34;b64&#34; ] ; then&#10;&#9;&#9;local file_name&#10;&#9;&#9;local SIFS=$IFS&#10;&#9;&#9;IFS=$'\\n'&#10;&#9;&#9;for file_name in $SOURCE_FILES&#10;&#9;&#9;do&#10;&#9;&#9;&#9;SOURCE_FILE0=$file_name&#10;&#9;&#9;&#9;break&#10;&#9;&#9;done&#10;&#9;&#9;IFS=$SIFS&#10;&#9;&#9;return 0&#10;&#9;fi&#10;&#9;if [ &#34;$ext&#34; == &#34;NOERROR&#34; ] ; then&#10;&#9;&#9;return 1&#10;&#9;fi&#10;&#10;&#9;echo &#34;To run this type of program you need some file with extension \\&#34;$@\\&#34;&#34;&#10;&#9;exit 0;&#10;}&#10;&#10;# Take SOURCE_FILES and write at $1 file&#10;function generate_file_of_files {&#10;&#9;if [ -f &#34;$1&#34; ] ; then&#10;&#9;&#9;rm &#34;$1&#34;&#10;&#9;fi&#10;&#9;touch $1 &#10;&#9;local file_name&#10;&#9;local SIFS=$IFS&#10;&#9;IFS=$'\\n'&#10;&#9;for file_name in $SOURCE_FILES&#10;&#9;do&#10;&#9;&#9;if [ &#34;$2&#34; == &#34;&#34; ] ; then&#10;&#9;&#9;&#9;echo &#34;\\&#34;$file_name\\&#34;&#34; &#62;&#62; &#34;$1&#34;&#10;&#9;&#9;else&#10;&#9;&#9;&#9;echo &#34;$file_name&#34; &#62;&#62; &#34;$1&#34;&#10;&#9;&#9;fi&#10;&#9;done&#10;&#9;IFS=$SIFS&#10;}&#10;&#10;# Set FIRST_SOURCE_FILE to the first VPL_SUBFILE# with extension in parameters $@&#10;function get_first_source_file {&#10;&#9;local ext&#10;&#9;local FILENAME&#10;&#9;local FILEVAR&#10;&#9;local i&#10;&#9;for i in {0..100000}&#10;&#9;do&#10;&#9;&#9;FILEVAR=&#34;VPL_SUBFILE${i}&#34;&#10;&#9;&#9;FILENAME=&#34;${!FILEVAR}&#34;&#10;&#9;&#9;if [ &#34;&#34; == &#34;$FILENAME&#34; ] ; then&#10;&#9;&#9;&#9;break&#10;&#9;&#9;fi&#10;&#9;&#9;for ext in &#34;$@&#34;&#10;&#9;&#9;do&#10;&#9;&#9;    if [ &#34;${FILENAME##*.}&#34; == &#34;$ext&#34; ] ; then&#10;&#9;&#9;        FIRST_SOURCE_FILE=$FILENAME&#10;&#9;&#9;        return 0&#10;&#9;    &#9;fi&#10;&#9;&#9;done&#10;&#9;done&#10;&#9;if [ &#34;$ext&#34; == &#34;NOERROR&#34; ] ; then&#10;&#9;&#9;return 1&#10;&#9;fi&#10;&#9;echo &#34;To run this type of program you need some file with extension \\&#34;$@\\&#34;&#34;&#10;&#9;exit 0;&#10;}&#10;&#10;# Check program existence ($@) and set $PROGRAM and PROGRAMPATH&#10;function check_program {&#10;&#9;PROGRAM=&#10;&#9;local check&#10;&#9;for check in &#34;$@&#34;&#10;&#9;do&#10;&#9;&#9;local PROPATH=$(command -v $check)&#10;&#9;&#9;if [ &#34;$PROPATH&#34; == &#34;&#34; ] ; then&#10;&#9;&#9;&#9;continue&#10;&#9;&#9;fi&#10;&#9;&#9;PROGRAM=$check&#10;&#9;&#9;PROGRAMPATH=$PROPATH&#10;&#9;&#9;return 0&#10;&#9;done&#10;&#9;if [ &#34;$check&#34; == &#34;NOERROR&#34; ] ; then&#10;&#9;&#9;return 1&#10;&#9;fi&#10;&#9;echo &#34;The execution server needs to install \\&#34;$1\\&#34; to run this type of program&#34;&#10;&#9;exit 0;&#10;}&#10;&#10;# Compile &#10;function compile_typescript {&#10;&#9;check_program tsc NOERROR&#10;&#9;if [ &#34;$PROGRAM&#34; == &#34;&#34; ] ; then&#10;&#9;&#9;return 0&#10;&#9;fi&#10;&#9;get_source_files ts NOERROR&#10;&#9;SAVEIFS=$IFS&#10;&#9;IFS=$'\\n'&#10;&#9;for FILENAME in $SOURCE_FILES&#10;&#9;do&#10;&#9;&#9;tsc &#34;$FILENAME&#34; | sed 's/\\x1b\\[[0-9;]*[a-zA-Z]//g'&#10;&#9;done&#10;&#9;IFS=$SAVEIFS&#10;}&#10;&#10;function compile_scss {&#10;&#9;check_program sass NOERROR&#10;&#9;if [ &#34;$PROGRAM&#34; == &#34;&#34; ] ; then&#10;&#9;&#9;return 0&#10;&#9;fi&#10;&#9;get_source_files scss NOERROR&#10;&#9;SAVEIFS=$IFS&#10;&#9;IFS=$'\\n'&#10;&#9;for FILENAME in $SOURCE_FILES&#10;&#9;do&#10;&#9;&#9;sass &#34;$FILENAME&#34;&#10;&#9;done&#10;&#9;IFS=$SAVEIFS&#10;}&#10;&#10;&#10;#Decode BASE64 files&#10;get_source_files b64&#10;SAVEIFS=$IFS&#10;IFS=$'\\n'&#10;for FILENAME in $SOURCE_FILES&#10;do&#10;&#9;if [ -f &#34;$FILENAME&#34; ] ; then&#10;&#9;&#9;BINARY=$(echo &#34;$FILENAME&#34; | sed -r &#34;s/\\.b64$//&#34;)&#10;&#9;&#9;if [ ! -f  &#34;$BINARY&#34; ] ; then&#10;&#9;&#9;&#9;base64 -i -d &#34;$FILENAME&#34; &#62; &#34;$BINARY&#34;&#10;&#9;&#9;fi&#10;&#9;fi&#10;done&#10;SOURCE_FILES=&#34;&#34;&#10;#Security Check: pre_vpl_run.sh was submitted by a student?&#10;VPL_NS=true&#10;for FILENAME in $VPL_SUBFILES&#10;do&#10;&#9;if [ &#34;$FILENAME&#34; == &#34;pre_vpl_run.sh&#34; ] || [ &#34;$FILENAME&#34; == &#34;pre_vpl_run.sh.b64&#34; ] ; then&#10;&#9;&#9;VPL_NS=false&#10;&#9;&#9;break&#10;&#9;fi&#10;done&#10;IFS=$SAVEIFS&#10;if $VPL_NS ; then&#10;&#9;if [ -x pre_vpl_run.sh ] ; then&#10;&#9;&#9;./pre_vpl_run.sh&#10;&#9;fi&#10;fi&#10;";
		filecontent = XML::decodeXML(filecontent);
		assert(files["common_script.sh"]->getString() == filecontent);
		assert(files.size() == 4);

		mapstruct filestodelete = rpc.getFileToDelete();
		assert(filestodelete["vpl_run.sh"]->getLong() == 1);
		assert(filestodelete.size() == 1);

		mapstruct fileencoding = rpc.getFileEncoding();
		assert(fileencoding["vpl_run.sh"]->getLong() == 0);
		assert(fileencoding["a.cpp"]->getLong() == 0);
		assert(fileencoding["vpl_environment.sh"]->getLong() == 0);
		assert(fileencoding["common_script.sh"]->getLong() == 0);
		assert(fileencoding.size() == 4);
	}

	void testDirectrun() {
		string rawdata = Util::readFile("./xmlrpcdata/directrun.xml");
		XMLRPC rpc(rawdata);
		string method = rpc.getMethodName();
		assert(method == "directrun");
		mapstruct data = rpc.getData();
		assert(data["maxtime"]->getLong() == 1000000);
		assert(data["maxfilesize"]->getLong() == 1000000000);
		assert(data["maxmemory"]->getLong() == 1000000000);
		assert(data["maxprocesses"]->getLong() == 10000);
		assert(data["runscript"]->getString() == "");
		assert(data["debugscript"]->getString() == "");
		assert(data["userid"]->getString() == "3");
		assert(data["activityid"]->getString() == "4382");
		assert(data["execute"]->getString() == ".vpl_directrun.sh");
		assert(data["interactive"]->getLong() == 1);
		assert(data["lang"]->getString() == "en_US.UTF-8");
		assert(data["pluginversion"]->getLong() == 2021052513L);

		mapstruct files = rpc.getFiles();
		string filecontent;
		filecontent = "test file .vpl_directrun.sh";
		filecontent = XML::decodeXML(filecontent);
		assert(files[".vpl_directrun.sh"]->getString() == filecontent);
		filecontent = "test file a.c";
		filecontent = XML::decodeXML(filecontent);
		assert(files["a.c"]->getString() == filecontent);
		filecontent = "test file b.c";
		filecontent = XML::decodeXML(filecontent);
		assert(files["b.c"]->getString() == filecontent);
		assert(files.size() == 3);

		mapstruct filestodelete = rpc.getFileToDelete();
		assert(filestodelete[".vpl_directrun.sh"]->getLong() == 1);
		assert(filestodelete.size() == 1);

		mapstruct fileencoding = rpc.getFileEncoding();
		assert(fileencoding[".vpl_directrun.sh"]->getLong() == 0);
		assert(fileencoding["a.c"]->getLong() == 0);
		assert(fileencoding["b.c"]->getLong() == 0);
		assert(fileencoding.size() == 3);
	}

	void testEncodeXML(){
		string text, textEncoded;
		assert(XML::encodeXML("") == "");
		assert(XML::encodeXML("correcto") == "correcto");
		text = "text\n\rcommon with spaces \t\t and new lines correcto";
		textEncoded = text;
		assert(XML::encodeXML(text) == textEncoded);
		text = "\"<>&'";
		textEncoded = "&quot;&lt;&gt;&amp;&apos;";
		assert(XML::encodeXML(text) == textEncoded);
		text = "a \" b < c > d & ee '";
		textEncoded = "a &quot; b &lt; c &gt; d &amp; ee &apos;";
		assert(XML::encodeXML(text) == textEncoded);
	}

public:
	string name() {
		return "XMLRPC class";
	}
	void launch() {
		testAvailable();
		testGetresult();
		testStop();
		testRunning();
		testRequest();
		testDirectrun();
		testEncodeXML();
	}
};

XMLRPCTest xmlRpcTest;
