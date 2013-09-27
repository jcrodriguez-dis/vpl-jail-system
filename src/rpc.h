/**
 * version:		$Id: rpc.h,v 1.4 2011-01-04 12:00:00 juanca Exp $
 * package:		Part of vpl-xmlrpc-jail
 * copyright:	Copyright (C) 2009 Juan Carlos Rodríguez-del-Pino. All rights reserved.
 * license:		GNU/GPL, see LICENSE.txt or http://www.gnu.org/licenses/gpl-2.0.html
 **/
 
#ifndef RPC_INC_H
#define PPC_INC_H
#include "xml.h"
#include "util.h"

/**
 * Class to prepare and process XML-RPC messages
 */
class RPC{
public:
	/**
	 * return a ready response
	 */
	static string readyResponse(string status, int load, int maxtime, int maxfilesize,
								int maxmemory, int maxprocesses, string softinstalled){
		string response;
		response += "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
		response += "<methodResponse>\n";
		response += "<params>\n";
		response += "<param>\n";
		response += "<struct>\n";
		response += "<member><name>status</name>\n";
		response += "<value><string>" + XML::encodeXML(status) +"</string></value>\n";
		response += "</member>\n";
		response += "<member><name>load</name>\n";
		response += "<value><int>" + Util::itos(load) + "</int></value>\n";
		response += "</member>\n";
		response += "<member><name>maxtime</name>\n";
		response += "<value><int>" + Util::itos(maxtime) + "</int></value>\n";
		response += "</member>\n";
		response += "<member><name>maxfilesize</name>\n";
		response += "<value><int>" + Util::itos(maxfilesize) + "</int></value>\n";
		response += "</member>\n";
		response += "<member><name>maxmemory</name>\n";
		response += "<value><int>" + Util::itos(maxmemory) + "</int></value>\n";
		response += "</member>\n";
		response += "<member><name>maxprocesses</name>\n";
		response += "<value><int>" + Util::itos(maxprocesses) + "</int></value>\n";
		response += "</member>\n";
		response += "<member><name>softinstalled</name>\n";
		response += "<value><string>" + XML::encodeXML(softinstalled) + "</string></value>\n";
		response += "</member>\n";
		response += "</struct>\n";
		response += "</param>\n";
		response += "</params>\n";
		response += "</methodResponse>\n";
		return response;
	}

	/**
	 * return a execute response
	 */
	static string executeResponse(string compilation,string execution, bool executed){
		string response;
		response+="<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
		response+="<methodResponse>\n";
		response+="<params>\n";
		response+="<param>\n";
		response+="<struct>\n";
		response+="<member>\n";
		response+="<name>compilation</name>\n";
		response+="<value><string>"+XML::encodeXML(compilation)+"</string></value>\n";
		response+="</member>\n";
		response+="<member>\n";
		response+="<name>execution</name>\n";
		response+="<value><string>"+XML::encodeXML(execution)+"</string></value>\n";
		response+="</member>\n";
		response+="<member>\n";
		response+="<name>executed</name>\n";
		if(executed){
			response+="<value><int>1</int></value>\n";
		}else{
			response+="<value><int>0</int></value>\n";
		}
		response+="</member>\n";
		response+="</struct>\n";
		response+="</param>\n";
		response+="</params>\n";
		response+="</methodResponse>\n";
		return response;
	}
	/**
	 * return method call name
	 */
	static string methodName(const XML::TreeNode *root){
		if(root->getName() == "methodCall"){
			const XML::TreeNode *method=root->child("methodName");
			return method->getContent();
		}
		throw "RPC/XML methodName parse error";
	}
	/**
	 * return struct value as a mapstruct
	 */
	static mapstruct getStructMembers(const XML::TreeNode *st){
		if(st->getName() == "struct"){
			mapstruct ret;
			for(size_t i=0; i< st->nchild(); i++){
				ret[st->child(i)->child("name")->getContent()]=st->child(i)->child("value")->child(0);
			}
			return ret;
		}else if(st->getName() == "array" && st->nchild() == 0){
			//An empty array
			mapstruct ret;
			return ret;
		}
		syslog(LOG_ERR,"Expected struct/array(0) found %s",st->getName().c_str());
		throw "RPC/XML getStructMembers parse error";
	}
	/**
	 * return struct of method call data as mapstruct
	 */
	static mapstruct getData(const XML::TreeNode *root){
		if(root->getName() == "methodCall"){
			const XML::TreeNode *st=root->child("params")->child("param")->child("value")->child("struct");
			return getStructMembers(st);
		}
		throw "RPC/XML getData parse error";
	}
	/**
	 * return list of files as mapstruct
	 */
	static mapstruct getFiles(const XML::TreeNode *files){
		return getStructMembers(files);
	}

};
#endif
