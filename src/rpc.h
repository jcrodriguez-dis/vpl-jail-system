/**
 * package:		Part of vpl-jail-system
 * copyright:	Copyright (C) 2014 Juan Carlos Rodríguez-del-Pino. All rights reserved.
 * license:		GNU/GPL, see LICENSE.txt or http://www.gnu.org/licenses/gpl-3.0.html
 **/

#ifndef RPC_INC_H
#define PPC_INC_H

#include "xml.h"
#include "util.h"

/**
 * Class to prepare and process XML-RPC messages
 */
class RPC {
public:
	/**
	 * return a ready response
	 */
	virtual string availableResponse(string status, int load, int maxtime, long long maxfilesize,
			long long maxmemory, int maxprocesses, int secureport) = 0;

	virtual string requestResponse(const string adminticket,const string monitorticket,
			const string executionticket, int port,int secuport) = 0;

	virtual string getResultResponse(const string &compilation,const string & execution, bool executed,bool interactive) = 0;
	virtual string updateResponse(bool ok) = 0;
	virtual string runningResponse(bool running) = 0;
	virtual string stopResponse() = 0;
	/**
	 * return method call name
	 */
	virtual string methodName(const TreeNode *root){
		if(root->getName() == "methodCall"){
			const TreeNode *method=root->child("methodName");
			return method->getContent();
		}
		throw HttpException(badRequestCode
				,"RPC/XML methodName parse error");
	}
	/**
	 * return struct value as a mapstruct
	 */
	virtual mapstruct getStructMembers(const TreeNode *st){
		if(st->getName() == "struct"){
			mapstruct ret;
			for(size_t i=0; i< st->nchild(); i++){
				ret[st->child(i)->child("name")->getString()]=st->child(i)->child("value")->child(0);
			}
			return ret;
		}else if(st->getName() == "array" && st->nchild() == 0){
			//An empty array
			mapstruct ret;
			return ret;
		}
		syslog(LOG_ERR,"Expected struct/array(0) found %s",st->getName().c_str());
		throw HttpException(badRequestCode
				,"RPC/XML getStructMembers parse error");
	}
	/**
	 * return struct of method call data as mapstruct
	 */
	virtual mapstruct getData(const TreeNode *root){
		if(root->getName() == "methodCall"){
			const TreeNode *st=root->child("params")->child("param")->child("value")->child("struct");
			return getStructMembers(st);
		}
		throw HttpException(badRequestCode
				,"RPC/XML getData parse error");
	}
	/**
	 * return list of files as mapstruct
	 */
	virtual mapstruct getFiles(const TreeNode *files){
		return getStructMembers(files);
	}

};
#endif
