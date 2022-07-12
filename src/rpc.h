/**
 * package:		Part of vpl-jail-system
 * copyright:	Copyright (C) 2014 Juan Carlos Rodríguez-del-Pino. All rights reserved.
 * license:		GNU/GPL, see LICENSE.txt or http://www.gnu.org/licenses/gpl-3.0.html
 **/

#ifndef RPC_INC_H
#define RPC_INC_H

#include "xml.h"
#include "util.h"

/**
 * Class to prepare and process XML-RPC messages
 */
class RPC {
protected:
	TreeNode *root;
public:
	RPC() {
		this->root = NULL;
	}

	virtual ~RPC() {}

	/**
	 * return a ready response
	 */
	virtual string availableResponse(string status, int load, int maxtime, long long maxfilesize,
			long long maxmemory, int maxprocesses, int secureport) = 0;

	virtual string requestResponse(const string adminticket,const string monitorticket,
			const string executionticket, int port,int secuport) = 0;
	virtual string directRunResponse(const string homepath, const string adminticket,
			const string executionticket, int port,int secuport) = 0;
	virtual string getResultResponse(const string &compilation,const string & execution, bool executed,bool interactive) = 0;
	virtual string updateResponse(bool ok) = 0;
	virtual string runningResponse(bool running) = 0;
	virtual string stopResponse() = 0;
	/**
	 * return method call name
	 */
	virtual string getMethodName() = 0;
	/**
	 * return struct of method call data as mapstruct
	 */
	virtual mapstruct getData() = 0;
	/**
	 * return list of files as mapstruct
	 */
	virtual mapstruct getFiles() = 0;

	/**
	 * return list of files encoding as mapstruct
	 */
	virtual mapstruct getFileEncoding() = 0;

	/**
	 * return list of files to delete as mapstruct
	 */
	virtual mapstruct getFileToDelete() = 0;
};
#endif
