/**
 * package:		Part of vpl-jail-system
 * copyright:	Copyright (C) 2009 Juan Carlos Rodríguez-del-Pino. All rights reserved.
 * license:		GNU/GPL, see LICENSE.txt or http://www.gnu.org/licenses/gpl-3.0.html
 **/

#ifndef HTTPSERVER_INC_H
#define HTTPSERVER_INC_H
#include <string>
#include <map>
#include <stdint.h>
#include "socket.h"
using namespace std;

/**
 * class HttpJailServer
 * Read HTTP request from an open socket
 * Process headers
 * Send response
 */

class HttpJailServer {
	Socket *socket;
	bool authenticated;
public:
	HttpJailServer(Socket *s):socket(s), authenticated(false){}
	void validateRequest(string path);
	void setAuthenticated(){ authenticated = true; }
	string receive();
	string prepare_HTTP(int code, const string &codeText, const string &response, const bool headMethod = false, const string extraHeader = "");
	void send(int code, const string &codeText, const string &response, const bool headMethod = false, const string extraHeader = "");
	void sendRaw(const string &);
	void send200(const string &, const bool headMethod = false, const string extraHeader = "");
	void sendCode(CodeNumber, string text="");
};

#endif
