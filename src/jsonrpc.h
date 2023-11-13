/**
 * package:		Part of vpl-jail-system
 * copyright:	Copyright (C) 2022 Juan Carlos Rodríguez-del-Pino. All rights reserved.
 * license:		GNU/GPL, see LICENSE.txt or http://www.gnu.org/licenses/gpl-3.0.html
 **/

#ifndef JSONRPC_INC_H
#define JSONRPC_INC_H

#include "rpc.h"
#include "json.h"
#include "util.h"

/**
 * Class to prepare and process JSON-RPC messages
 */
class JSONRPC: public RPC {
protected:
	JSON *json;
	string method;
	string id;
	mapstruct data;
	void processRoot() {
		if(root->getName() != "object") {
			throw HttpException(badRequestCode,"JSONRPC root must be object");
		}
		this->method = this->root->child("method")->getString();
		this->data = getStructMembers(this->root->child("params"));
		this->id = this->root->child("id")->getString();
		if(root->nchild() == 4 && this->root->child("jsonrpc")->getString() == "2.0") {
			return;
		}
		if(root->nchild() != 3) { // For compatibility with buggy plugin
			throw HttpException(badRequestCode,"JSONRPC root must have 4 attributes");
		}

	}
public:
	JSONRPC(string rawData) {
		this->json = new JSON(rawData);
		this->root = json->getRoot();
		processRoot();
	}

	~JSONRPC() {
		delete this->json;
	}

	string responseWraper(string response) {
		return "{\"jsonrpc\":\"2.0\",\"result\":"
				+ response +
				",\"id\":\""
				+ this->getId() +
				"\"}";
	}
	string responseMember(const string name, const string & value) {
		return "\"" + name + "\":\"" + JSON::encodeJSONString(value) + "\"";
	}
	string responseMember(string name, long long value) {
		return "\"" + name + "\":" + Util::itos(value);
	}
	/**
	 * return a ready response
	 */
	string availableResponse(string status, int load, int maxtime, long long maxfilesize,
			long long maxmemory, int maxprocesses, int secureport) {
		string response = "{";
		response += responseMember("status", status) + ",";
		response += responseMember("load", load) + ",";
		response += responseMember("maxtime", maxtime) + ",";
		response += responseMember("maxfilesize", maxfilesize) + ",";
		response += responseMember("maxmemory", maxmemory) + ",";
		response += responseMember("maxprocesses", maxprocesses) + ",";
		response += responseMember("secureport", secureport) + "}";
		return responseWraper(response);
	}

	string requestResponse(const string adminticket,const string monitorticket,
			const string executionticket, int port,int secuport) {
		string response = "{";
		response += responseMember("adminticket", adminticket) + ",";
		response += responseMember("monitorticket", monitorticket) + ",";
		response += responseMember("executionticket", executionticket) + ",";
		response += responseMember("port", port) + ",";
		response += responseMember("secureport", secuport) + "}";
		return responseWraper(response);
	}

	string directRunResponse(const string homepath, const string adminticket,
							 const string executionticket, int port, int secuport) {
		string response = "{";
		response += responseMember("homepath", homepath) + ",";
		response += responseMember("adminticket", adminticket) + ",";
		response += responseMember("executionticket", executionticket) + ",";
		response += responseMember("port", port) + ",";
		response += responseMember("secureport", secuport) + "}";
		return responseWraper(response);
	}

	string getResultResponse(const string &compilation,const string & execution, bool executed,bool interactive) {
		string response = "{";
		response += responseMember("compilation", compilation) + ",";
		response += responseMember("execution", execution) + ",";
		response += responseMember("executed", (executed?1:0)) + ",";
		response += responseMember("interactive", (interactive?1:0)) + "}";
		return responseWraper(response);
	}
	string updateResponse(bool ok) {
		string response = "{";
		response += responseMember("update", ok? 1 : 0) + "}";
		return responseWraper(response);
	}
	string runningResponse(bool running) {
		string response = "{";
		response += responseMember("running", running? 1 : 0) + "}";
		return responseWraper(response);
	}
	string stopResponse() {
		string response = "{";
		response += responseMember("stop", 1) + "}";
		return responseWraper(response);
	}
	/**
	 * return method call name
	 */
	string getMethodName() {
		return this->method;
	}
	/**
	 * return request id
	 */
	string getId() {
		return this->id;
	}
	/**
	 * return struct value as a mapstruct
	 */
	static mapstruct getStructMembers(const TreeNode *st) {
		mapstruct ret;
		for(size_t i=0; i < st->nchild(); i++){
			ret[st->child(i)->getName()] = st->child(i);
		}
		return ret;
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
