/**
 * package:		Part of vpl-jail-system
 * copyright:	Copyright (C) 202" Juan Carlos Rodríguez-del-Pino. All rights reserved.
 * license:		GNU/GPL, see LICENSE.txt or http://www.gnu.org/licenses/gpl-3.0.html
 **/

#include "basetest.h"
#include <cassert>
#include "../src/jsonrpc.h"

class JSONRPCTest: public BaseTest {
	void testJSON() {
		string data, converted, encoded;
		data = "Hello";
		encoded = "Hello";
		assert(encoded == JSON::encodeJSONString(data));
		converted = JSON::decodeJSONString(JSON::encodeJSONString(data), 0, 10000);
		assert(data == converted);
		data = "Hello\nsome lines\n\t   \n \ralgo";
		encoded = "Hello\\nsome lines\\n\\t   \\n \\ralgo";
		assert(encoded == JSON::encodeJSONString(data));
		converted = JSON::decodeJSONString(JSON::encodeJSONString(data), 0, 10000);
		assert(data == converted);
		data = "\\\nsome \r\nlines\n\t   \n";
		encoded = "\\\\\\nsome \\r\\nlines\\n\\t   \\n";
		assert(encoded == JSON::encodeJSONString(data));
		converted = JSON::decodeJSONString(JSON::encodeJSONString(data), 0, 10000);
		assert(data == converted);
		data = "\\áéíóúÑñ";
		encoded = JSON::encodeJSONString(data);
		converted = JSON::decodeJSONString(encoded, 0, 10000);
		assert(data == converted);
		encoded = "\\\\\\u00e1\\u00e9\\u00ed\\u00f3\\u00fa\\u00d1\\u00f1";
		converted = JSON::decodeJSONString(encoded, 0, 10000);
		assert(data == converted);
		data = "😀😇👺🤜🔲🔢";
		converted = JSON::decodeJSONString(JSON::encodeJSONString(data), 0, 10000);
		assert(data == converted);
		encoded = "\\ud83d\\ude00\\ud83d\\ude07\\ud83d\\udc7a"
		 		  "\\ud83e\\udd1c\\ud83d\\udd32\\ud83d\\udd22";
		converted = JSON::decodeJSONString(encoded, 0, 10000);
		// Check control codes conversion
		assert(data == converted);
		char c_data[33];
		for (int i = 0; i < 31; i++) c_data[i] = (char) (i+1);
		c_data[31] = 127;
		c_data[32] = 0;
		converted = JSON::decodeJSONString(JSON::encodeJSONString(c_data), 0, 10000);
		assert(converted == c_data);
	}

	void testAvailable() {
		string rawdata = Util::readFile("./jsondata/available.json");
		JSONRPC rpc(rawdata);
		string method = rpc.getMethodName();
		assert(method == "available");
		mapstruct data = rpc.getData();
		assert(data["maxmemory"]->getLong() == 1073741824);
		string reponseString = rpc.availableResponse("OK", 3, 333, 456678,5555555, 300, 443);
		JSON responseJSON(reponseString);
		TreeNode* root = responseJSON.getRoot();
		assert(root->child("jsonrpc")->getString() == "2.0");
		assert(root->child("id")->getString() == rpc.getId());
		TreeNode* response = root->child("result");
		assert( "OK" == response->child("status")->getString());
		assert( 3 == response->child("load")->getLong());
		assert( 333 == response->child("maxtime")->getLong());
		assert( 456678 == response->child("maxfilesize")->getLong());
		assert( 5555555 == response->child("maxmemory")->getLong());
		assert( 300 == response->child("maxprocesses")->getLong());
		assert( 443 == response->child("secureport")->getLong());
	}

	string innerTest(string file, string method, string adminticket, long long pluginversion, string id) {
		string rawdata = Util::readFile("./jsondata/" + file);
		JSONRPC rpc(rawdata);
		assert(rpc.getMethodName() == method);
		mapstruct data = rpc.getData();
		assert(data["adminticket"]->getString() == adminticket);
		assert(data["pluginversion"]->getLong() == pluginversion);
		assert(rpc.getId() == id);
		return rawdata;
	}

	void testGetresult() {
		string raw = innerTest("getresult.json", "getresult", "82955023616526", 2021061600L, "3-79893-117530400");
		JSONRPC rpc(raw);
		string reponseString = rpc.getResultResponse("Compilation\n\nend\n", "execution result", true, false);
		JSON responseJSON(reponseString);
		TreeNode* root = responseJSON.getRoot();
		assert(root->child("jsonrpc")->getString() == "2.0");
		assert(root->child("id")->getString() == rpc.getId());
		TreeNode* response = root->child("result");
		assert( "Compilation\n\nend\n" == response->child("compilation")->getString());
		assert( "execution result" == response->child("execution")->getString());
		assert( 1 == response->child("executed")->getLong());
		assert( 0 == response->child("interactive")->getLong());
	}

	void testStop() {
		string raw = innerTest("stop.json", "stop", "143542895320808", 2021061600L, "3-79866-543371100");
		JSONRPC rpc(raw);
		string reponseString = rpc.stopResponse();
		JSON responseJSON(reponseString);
		TreeNode* root = responseJSON.getRoot();
		assert(root->child("jsonrpc")->getString() == "2.0");
		assert(root->child("id")->getString() == rpc.getId());
		TreeNode* response = root->child("result");
		assert( 1 == response->child("stop")->getLong());
	}

	void testRunning() {
		string raw = innerTest("running.json", "running", "336135110306607", 2021061600L, "3-79881-245914000");
		JSONRPC rpc(raw);
		string reponseString = rpc.runningResponse(true);
		JSON responseJSON(reponseString);
		TreeNode* root = responseJSON.getRoot();
		assert(root->child("jsonrpc")->getString() == "2.0");
		assert(root->child("id")->getString() == rpc.getId());
		TreeNode* response = root->child("result");
		assert( 1 == response->child("running")->getInt());
		JSON responseJSONFalse(rpc.runningResponse(false));
		assert( 0 == responseJSONFalse.getRoot()->child("result")->child("running")->getLong());
	}

	void testRequest() {
		string rawdata = Util::readFile("./jsondata/request.json");
		JSONRPC rpc(rawdata);
		
		string method = rpc.getMethodName();
		assert(rpc.getId() == "3-79866-684945600");
		assert(method == "request");
		mapstruct data = rpc.getData();
		assert(data["maxtime"]->getLong() == 960);
		assert(data["maxfilesize"]->getLong() == 67108864);
		assert(data["maxmemory"]->getLong() == 1073741824);
		assert(data["maxprocesses"]->getLong() == 200);
		assert(data["runscript"]->getString() == "null");
		assert(data["debugscript"]->getString() == "c");
		assert(data["userid"]->getString() == "3");
		assert(data["activityid"]->getString() == "7");
		assert(data["execute"]->getString() == "vpl_run.sh");
		assert(data["interactive"]->getInt() == 1);
		assert(data["lang"]->getString() == "en_US.UTF-8");
		assert(data["pluginversion"]->getLong() == 2021061600L);

		mapstruct files = rpc.getFiles();
		string correct, calculated;
		correct = "int main(){\n    system(\"/bin/bash\");\n}";
		assert(files["a.c"]->getString() == correct);
		correct = "void f() {\n}\n";
		assert(files["b.c"]->getString() == correct);
		correct = "#!/bin/bash\n# This file is part of VPL for Moodle - http://vpl.dis.ulpgc.es/\n# Script for running C language \n# Copyright (C) 2014 Juan Carlos Rodríguez-del-Pino\n# License http://www.gnu.org/copyleft/gpl.html GNU GPL v3 or later\n# Author Juan Carlos Rodríguez-del-Pino <jcrodriguez@dis.ulpgc.es>\n\n# @vpl_script_description Using GCC with math and util libs\n# load common script and check programs\n. common_script.sh\ncheck_program gcc\nif [ \"$1\" == \"version\" ] ; then\n\techo \"#!/bin/bash\" > vpl_execution\n\techo \"gcc --version | head -n2\" >> vpl_execution\n\tchmod +x vpl_execution\n\texit\nfi \nget_source_files c\n# Generate file with source files\ngenerate_file_of_files .vpl_source_files\n# Compile\neval gcc -fno-diagnostics-color -o vpl_execution $2 @.vpl_source_files -lm -lutil \nrm .vpl_source_files\n";
		calculated = files["vpl_run.sh"]->getString();
		assert(calculated == correct);
		correct = "#!/bin/bash\nexport VPL_LANG='en_US.UTF-8'\nexport MOODLE_USER_ID='3'\nexport MOODLE_USER_NAME='Juan Carlos Rodríguez del Pino'\nexport MOODLE_USER_EMAIL='jcrodriguez@dis.ulpgc.es'\nexport VPL_SUBFILE0='a.c'\nexport VPL_SUBFILE1='b.c'\nexport VPL_SUBFILES=\"a.c\nb.c\n\"\nexport VPL_VARIATION=''\n\nexport VPL_XGEOMETRY='1386x766'\n";
		assert(files["vpl_environment.sh"]->getString() == correct);
		correct = "#!/bin/bash\n# Default common funtions for scripts of VPL\n# Copyright (C) 2016 Juan Carlos Rodríguez-del-Pino\n# License http://www.gnu.org/copyleft/gpl.html GNU GPL v3 or later\n# Author Juan Carlos Rodríguez-del-Pino <jcrodriguez@dis.ulpgc.es>\n\n#load VPL environment vars\nif [ \"$PROFILE_RUNNED\" == \"\" ] ; then\n\texport PROFILE_RUNNED=yes\n\tif [ -f /etc/profile ] ; then\n\t\tcp /etc/profile .localvplprofile\n\t\tchmod +x .localvplprofile\n\t\t. .localvplprofile\n\t\trm .localvplprofile\n\tfi\nfi\n. vpl_environment.sh\n#Use current lang\nexport LC_ALL=$VPL_LANG 1>/dev/null 2>vpl_set_locale_error\n#If current lang not available use en_US.UTF-8\nif [ -s vpl_set_locale_error ] ; then\n\texport LC_ALL=en_US.UTF-8  1>/dev/null 2>/dev/null\nfi\nrm vpl_set_locale_error 1>/dev/null 2>/dev/null\n#functions\n\n# Wait until a program ($1 e.g. execution_int) of the current user ends. \nfunction wait_end {\n\tlocal PSRESFILE\n\tPSRESFILE=.vpl_temp_search_program\n\t#wait start until 5s\n\tfor I in 1 .. 5\n\tdo\n\t\tsleep 1s\n\t\tps -f -u $USER > $PSRESFILE\n\t\tgrep $1 $PSRESFILE &> /dev/null\n\t\tif [ \"$?\" == \"0\" ] ; then\n\t\t\tbreak\n\t\tfi\n\tdone\n\twhile :\n\tdo\n\t\tsleep 1s\n\t\tps -f -u $USER > $PSRESFILE\n\t\tgrep $1 $PSRESFILE &> /dev/null\n\t\tif [ \"$?\" != \"0\" ] ; then\n\t\t\trm $PSRESFILE\n\t\t\treturn\n\t\tfi\n\tdone\n}\n\n# Adds code to vpl_execution for getting the version of $PROGRAM\n# $1: version command line switch (e.g. -version)\n# $2: number of lines to show. Default 2\nfunction get_program_version {\n\tlocal nhl\n\tif [ \"$2\" == \"\" ] ; then\n\t\tnhl=2\n\telse\n\t\tnhl=$2\n\tfi\n\t\n\techo \"#!/bin/bash\" > vpl_execution\n\tif [ \"$1\" == \"unknown\" ] ; then\n\t\techo \"echo \\\"$PROGRAM version unknown\\\"\" >> vpl_execution\n\telse\n\t\techo \"$PROGRAM $1 | head -n $nhl\" >> vpl_execution\n\tfi\n\tchmod +x vpl_execution\n\texit\n}\n\n# Populate SOURCE_FILES, SOURCE_FILES_LINE and SOURCE_FILE0 with files\n# of extensions passed. E.g. get_source_files cpp C\nfunction get_source_files {\n\tlocal ext\n\tSOURCE_FILES=\"\"\n\tSOURCE_FILES_LINE=\"\"\n\tfor ext in \"$@\"\n\tdo\n\t\tif [ \"$ext\" == \"NOERROR\" ] ; then\n\t\t\tbreak\n\t\tfi\n\t    local source_files_ext=\"$(find . -name \"*.$ext\" -print | sed 's/^.\\///g' | sed 's/ /\\\\ /g')\"\n\t    if [ \"$SOURCE_FILES_LINE\" == \"\" ] ; then\n\t        SOURCE_FILES_LINE=\"$source_files_ext\"\n\t    else\n\t        SOURCE_FILES_LINE=$(echo -en \"$SOURCE_FILES_LINE\\n$source_files_ext\")\n\t    fi\n\t    local source_files_ext_s=\"$(find . -name \"*.$ext\" -print | sed 's/^.\\///g')\"\n\t    if [ \"$SOURCE_FILES\" == \"\" ] ; then\n\t        SOURCE_FILES=\"$source_files_ext_s\"\n\t    else\n\t        SOURCE_FILES=$(echo -en \"$SOURCE_FILES\\n$source_files_ext_s\")\n\t    fi\n\tdone\n\n    if [ \"$SOURCE_FILES\" != \"\" -o \"$1\" == \"b64\" ] ; then\n\t\tlocal file_name\n\t\tlocal SIFS=$IFS\n\t\tIFS=$'\\n'\n\t\tfor file_name in $SOURCE_FILES\n\t\tdo\n\t\t\tSOURCE_FILE0=$file_name\n\t\t\tbreak\n\t\tdone\n\t\tIFS=$SIFS\n\t\treturn 0\n\tfi\n\tif [ \"$ext\" == \"NOERROR\" ] ; then\n\t\treturn 1\n\tfi\n\n\techo \"To run this type of program you need some file with extension \\\"$@\\\"\"\n\texit 0;\n}\n\n# Take SOURCE_FILES and write at $1 file\nfunction generate_file_of_files {\n\tif [ -f \"$1\" ] ; then\n\t\trm \"$1\"\n\tfi\n\ttouch $1 \n\tlocal file_name\n\tlocal SIFS=$IFS\n\tIFS=$'\\n'\n\tfor file_name in $SOURCE_FILES\n\tdo\n\t\tif [ \"$2\" == \"\" ] ; then\n\t\t\techo \"\\\"$file_name\\\"\" >> \"$1\"\n\t\telse\n\t\t\techo \"$file_name\" >> \"$1\"\n\t\tfi\n\tdone\n\tIFS=$SIFS\n}\n\n# Set FIRST_SOURCE_FILE to the first VPL_SUBFILE# with extension in parameters $@\nfunction get_first_source_file {\n\tlocal ext\n\tlocal FILENAME\n\tlocal FILEVAR\n\tlocal i\n\tfor i in {0..100000}\n\tdo\n\t\tFILEVAR=\"VPL_SUBFILE${i}\"\n\t\tFILENAME=\"${!FILEVAR}\"\n\t\tif [ \"\" == \"$FILENAME\" ] ; then\n\t\t\tbreak\n\t\tfi\n\t\tfor ext in \"$@\"\n\t\tdo\n\t\t    if [ \"${FILENAME##*.}\" == \"$ext\" ] ; then\n\t\t        FIRST_SOURCE_FILE=$FILENAME\n\t\t        return 0\n\t    \tfi\n\t\tdone\n\tdone\n\tif [ \"$ext\" == \"NOERROR\" ] ; then\n\t\treturn 1\n\tfi\n\techo \"To run this type of program you need some file with extension \\\"$@\\\"\"\n\texit 0;\n}\n\n# Check program existence ($@) and set $PROGRAM and PROGRAMPATH\nfunction check_program {\n\tPROGRAM=\n\tlocal check\n\tfor check in \"$@\"\n\tdo\n\t\tlocal PROPATH=$(command -v $check)\n\t\tif [ \"$PROPATH\" == \"\" ] ; then\n\t\t\tcontinue\n\t\tfi\n\t\tPROGRAM=$check\n\t\tPROGRAMPATH=$PROPATH\n\t\treturn 0\n\tdone\n\tif [ \"$check\" == \"NOERROR\" ] ; then\n\t\treturn 1\n\tfi\n\techo \"The execution server needs to install \\\"$1\\\" to run this type of program\"\n\texit 0;\n}\n\n# Compile \nfunction compile_typescript {\n\tcheck_program tsc NOERROR\n\tif [ \"$PROGRAM\" == \"\" ] ; then\n\t\treturn 0\n\tfi\n\tget_source_files ts NOERROR\n\tSAVEIFS=$IFS\n\tIFS=$'\\n'\n\tfor FILENAME in $SOURCE_FILES\n\tdo\n\t\ttsc \"$FILENAME\" | sed 's/\\x1b\\[[0-9;]*[a-zA-Z]//g'\n\tdone\n\tIFS=$SAVEIFS\n}\n\nfunction compile_scss {\n\tcheck_program sass NOERROR\n\tif [ \"$PROGRAM\" == \"\" ] ; then\n\t\treturn 0\n\tfi\n\tget_source_files scss NOERROR\n\tSAVEIFS=$IFS\n\tIFS=$'\\n'\n\tfor FILENAME in $SOURCE_FILES\n\tdo\n\t\tsass \"$FILENAME\"\n\tdone\n\tIFS=$SAVEIFS\n}\n\n\n#Decode BASE64 files\nget_source_files b64\nSAVEIFS=$IFS\nIFS=$'\\n'\nfor FILENAME in $SOURCE_FILES\ndo\n\tif [ -f \"$FILENAME\" ] ; then\n\t\tBINARY=$(echo \"$FILENAME\" | sed -r \"s/\\.b64$//\")\n\t\tif [ ! -f  \"$BINARY\" ] ; then\n\t\t\tbase64 -i -d \"$FILENAME\" > \"$BINARY\"\n\t\tfi\n\tfi\ndone\nSOURCE_FILES=\"\"\n#Security Check: pre_vpl_run.sh was submitted by a student?\nVPL_NS=true\nfor FILENAME in $VPL_SUBFILES\ndo\n\tif [ \"$FILENAME\" == \"pre_vpl_run.sh\" ] || [ \"$FILENAME\" == \"pre_vpl_run.sh.b64\" ] ; then\n\t\tVPL_NS=false\n\t\tbreak\n\tfi\ndone\nIFS=$SAVEIFS\nif $VPL_NS ; then\n\tif [ -x pre_vpl_run.sh ] ; then\n\t\t./pre_vpl_run.sh\n\tfi\nfi\n";
		assert(files["common_script.sh"]->getString() == correct);
		assert(files.size() == 5);
		mapstruct filestodelete = rpc.getFileToDelete();
		assert(filestodelete["vpl_run.sh"]->getInt() == 1);
		assert(filestodelete.size() == 1);
		mapstruct fileencoding = rpc.getFileEncoding();
		assert(fileencoding["vpl_run.sh"]->getLong() == 0);
		assert(fileencoding["a.c"]->getLong() == 0);
		assert(fileencoding["b.c"]->getLong() == 0);
		assert(fileencoding["vpl_environment.sh"]->getLong() == 0);
		assert(fileencoding["common_script.sh"]->getLong() == 0);
		assert(fileencoding.size() == 5);

		string reponseString = rpc.requestResponse("sfdkjhkwehrweisdfh", "ksdfha fkjsdahfkjasd", "uiekhf\n\\ldsfkj", 8000, 9000);
		JSON responseJSON(reponseString);
		TreeNode* root = responseJSON.getRoot();
		assert(root->child("jsonrpc")->getString() == "2.0");
		assert(root->child("id")->getString() == rpc.getId());
		TreeNode* response = root->child("result");
		assert( "sfdkjhkwehrweisdfh" == response->child("adminticket")->getString());
		assert( "ksdfha fkjsdahfkjasd" == response->child("monitorticket")->getString());
		assert( "uiekhf\n\\ldsfkj" == response->child("executionticket")->getString());
		assert( 8000 == response->child("port")->getLong());
		assert( 9000 == response->child("secureport")->getLong());
	}

	void testDirectrun() {
		string rawdata = Util::readFile("./jsondata/directrun.json");
		JSONRPC rpc(rawdata);
		
		string method = rpc.getMethodName();
		assert(rpc.getId() == "3-79866-684945600");
		assert(method == "directrun");
		mapstruct data = rpc.getData();
		assert(data["maxtime"]->getLong() == 1000000);
		assert(data["maxfilesize"]->getLong() == 1000000000);
		assert(data["maxmemory"]->getLong() == 1000000000);
		assert(data["maxprocesses"]->getLong() == 10000);
		assert(data["execute"]->getString() == ".vpl_directrun.sh");
		assert(data["interactive"]->getInt() == 1);
		assert(data["lang"]->getString() == "en_US.UTF-8");
		assert(data["pluginversion"]->getLong() == 2021061600L);

		mapstruct files = rpc.getFiles();
		string correct, calculated;
		correct = "test file a.c";
		assert(files["a.c"]->getString() == correct);
		correct = "test file b.c";
		assert(files["b.c"]->getString() == correct);
		correct = "test file .vpl_directrun.sh";
		calculated = files[".vpl_directrun.sh"]->getString();
		assert(calculated == correct);
		assert(files.size() == 3);
		mapstruct filestodelete = rpc.getFileToDelete();
		assert(filestodelete[".vpl_directrun.sh"]->getInt() == 1);
		assert(filestodelete.size() == 1);
		mapstruct fileencoding = rpc.getFileEncoding();
		assert(fileencoding[".vpl_directrun.sh"]->getLong() == 0);
		assert(fileencoding["a.c"]->getLong() == 0);
		assert(fileencoding["b.c"]->getLong() == 0);
		assert(fileencoding.size() == 3);

		string reponseString = rpc.directRunResponse("/home/p12003", "sfdkjhkwehrweisdfh", "uiekhf\n\\ldsfkj", 8000, 9000);
		JSON responseJSON(reponseString);
		TreeNode* root = responseJSON.getRoot();
		assert(root->child("jsonrpc")->getString() == "2.0");
		assert(root->child("id")->getString() == rpc.getId());
		TreeNode* response = root->child("result");
		assert( "/home/p12003" == response->child("homepath")->getString());
		assert( "sfdkjhkwehrweisdfh" == response->child("adminticket")->getString());
		assert( "uiekhf\n\\ldsfkj" == response->child("executionticket")->getString());
		assert( 8000 == response->child("port")->getLong());
		assert( 9000 == response->child("secureport")->getLong());
	}

public:
	string name() {
		return "JSONRPC class";
	}
	void launch() {
		testJSON();
		testAvailable();
		testGetresult();
		testStop();
		testRunning();
		testRequest();
		testDirectrun();
	}
};

JSONRPCTest jsonRpcTest;
