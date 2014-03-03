/**
 * version:		$Id: httpServer.h,v 1.9 2014-02-21 14:27:33 juanca Exp $
 * package:		Part of vpl-jail-system
 * copyright:	Copyright (C) 2009 Juan Carlos Rodríguez-del-Pino. All rights reserved.
 * license:		GNU/GPL, see LICENSE.txt or http://www.gnu.org/licenses/gpl-3.0.html
 **/

#ifndef HTTPSERVER_INC_H
#define HTTPSERVER_INC_H
#include <string>
#include <map>
using namespace std;
#include <stdint.h>
#include "socket.h"

/**
 * class HttpJailServer
 * Read http request from an open socket
 * Process headers
 * Send response
 */

class HttpJailServer{
	Socket *socket;
public:
	HttpJailServer(Socket *s):socket(s){}
	void validateRequest(string path);
	string receive();
	void send(int code, const string &codeText, const string &response);
	void sendRaw(const string &);
	void send200(const string &);
	void sendCode(CodeNumber, string text="");
};

#endif
