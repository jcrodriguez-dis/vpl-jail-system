/**
 * package:		Part of vpl-jail-system
 * copyright:	Copyright (C) 2022 Juan Carlos Rodríguez-del-Pino. All rights reserved.
 * license:		GNU/GPL, see LICENSE.txt or http://www.gnu.org/licenses/gpl-3.0.html
 **/

#ifndef RPC_INC_H
#define PPC_INC_H

#include "rpc.h"
#include "xml.h"
#include "util.h"

/**
 * Class to prepare and process XML-RPC messages
 */
class XMLRPC: public RPC {
	XML *xml;
	TreeNode *method;
	mapstruct data;
	void processRoot() {
		if(root->getName() == "methodCall"){
			if(root->nchild() != 2) {
				throw HttpException(badRequestCode,"RPC/XML root must have 2 attributes");
			}
			this->method = this->root->child("methodName");
			this->data = getStructMembers(this->root->child("params")->child("param")->child("value")->child("struct"));
		} else {
			throw HttpException(badRequestCode, "RPC/XML methodName parse error");
		}
	}
public:
	XMLRPC(string rawData) {
		this->xml = new XML(rawData);
		this->root = xml->getRoot();
		processRoot();
	}

	~XMLRPC() {
		delete this->xml;
	}

	string responseWraper(string response) {
		return "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
				"<methodResponse>\n"
				"<params>\n"
				"<param>\n"
				"<struct>\n"
				+response+
				"</struct>\n"
				"</param>\n"
				"</params>\n"
				"</methodResponse>\n";
	}
	string responseMember(const string name, const string & value) {
		return "<member><name>"
				+XML::encodeXML(name)+"</name>\n"
				"<value><string>"
				+ XML::encodeXML(value) +
				"</string></value>\n"
				"</member>\n";
	}
	string responseMember(string name, long long value) {
		if (value <= INT32_MAX) {
			return "<member><name>"
					+name+"</name>\n"
					"<value><int>"
					+ Util::itos(value) +
					"</int></value>\n"
					"</member>\n";
		} else {
			return "<member><name>"
					+name+"</name>\n"
					"<value><double>"
					+ Util::dtos(value) +
					"</double></value>\n"
					"</member>\n";			
		}
	}
	/**
	 * return a ready response
	 */
	string availableResponse(string status, int load, int maxtime, long long maxfilesize,
			long long maxmemory, int maxprocesses, int secureport) {
		string response;
		response += responseMember("status", status);
		response += responseMember("load", load);
		response += responseMember("maxtime", maxtime);
		response += responseMember("maxfilesize", maxfilesize);
		response += responseMember("maxmemory", maxmemory);
		response += responseMember("maxprocesses", maxprocesses);
		response += responseMember("secureport", secureport);
		return responseWraper(response);
	}

	string requestResponse(const string adminticket,const string monitorticket,
			const string executionticket, int port,int secuport) {
		string response;
		response += responseMember("adminticket", adminticket);
		response += responseMember("monitorticket", monitorticket);
		response += responseMember("executionticket", executionticket);
		response += responseMember("port", port);
		response += responseMember("secureport", secuport);
		return responseWraper(response);
	}

	string directRunResponse(const string homepath, const string adminticket,
							 const string executionticket, int port,int secuport) {
		string response;
		response += responseMember("homepath", homepath);
		response += responseMember("adminticket", adminticket);
		response += responseMember("executionticket", executionticket);
		response += responseMember("port", port);
		response += responseMember("secureport", secuport);
		return responseWraper(response);
	}


	string getResultResponse(const string &compilation,const string & execution, bool executed,bool interactive) {
		string response;
		response += responseMember("compilation",compilation);
		response += responseMember("execution",execution);
		response += responseMember("executed",(executed?1:0));
		response += responseMember("interactive",(interactive?1:0));
		return responseWraper(response);
	}
	string updateResponse(bool ok) {
		string response;
		response += responseMember("update", ok? 1 : 0);
		return responseWraper(response);
	}
	string runningResponse(bool running) {
		string response;
		response += responseMember("running", running? 1 : 0);
		return responseWraper(response);
	}
	string stopResponse() {
		string response;
		response += responseMember("stop",1);
		return responseWraper(response);
	}
	/**
	 * return method call name
	 */
	string getMethodName() {
		return this->method->getRawContent();
	}
	/**
	 * return struct value as a mapstruct
	 */
	static mapstruct getStructMembers(const TreeNode *st) {
		if(st->getName() == "struct"){
			mapstruct ret;
			for(size_t i=0; i< st->nchild(); i++){
				ret[st->child(i)->child("name")->getString()] = st->child(i)->child("value")->child(0);
			}
			return ret;
		}else if(st->getName() == "array" && st->nchild() == 0) {
			//An empty array
			mapstruct ret;
			return ret;
		}
		Logger::log(LOG_ERR,"Expected struct/array(0) found %s",st->getName().c_str());
		throw HttpException(badRequestCode, "RPC/XML getStructMembers parse error " + st->getName() + " " + st->getRawContent());
	}
	/**
	 * return struct of method call data as mapstruct
	 */
	mapstruct getData() {
		return this->data;
	}
	/**
	 * return list of files as mapstruct
	 */
	mapstruct getFiles() {
		return getStructMembers(this->data["files"]);
	}
	/**
	 * return list of files encoding as mapstruct
	 */
	mapstruct getFileEncoding() {
		if (this->data.find("fileencoding") != this->data.end()) {
			return getStructMembers(this->data["fileencoding"]);
		} else {
			return mapstruct();
		}
	}

	/**
	 * return list of files to delete as mapstruct
	 */
	mapstruct getFileToDelete() {
		if (this->data.find("filestodelete") != this->data.end()) {
			return getStructMembers(this->data["filestodelete"]);
		} else {
			return mapstruct();
		}
	}

};
#endif
