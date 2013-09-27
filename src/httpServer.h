/**
 * version:		$Id: httpServer.h,v 1.4 2011-06-07 08:57:53 juanca Exp $
 * package:		Part of vpl-xmlrpc-jail
 * copyright:	Copyright (C) 2009 Juan Carlos Rodríguez-del-Pino. All rights reserved.
 * license:		GNU/GPL, see LICENSE.txt or http://www.gnu.org/licenses/gpl-2.0.html
 **/

#ifndef HTTPSERVER_INC_H
#define HTTPSERVER_INC_H
#include <string>
#include <map>
using namespace std;
#include <stdint.h>
const int MAXDATA=100000000;
/**
 * class HttpJailServer
 * Read http request from an open socket
 * Process headers
 * Send response
 */
class HttpJailServer{
	int socket;			//open socket connect to client
	uint32_t clientip;	//client ip taken from socket
	string URLPath;			//URL path expected
	map<string,string> headers;		//Headers already parses
	size_t maxDataSize;	//Max program data size to read
	size_t sizeToRead;	//size to read taken from headers
	void parseRequestLine(string line, string &method, string &URL, string &version);
	void parseHeader(const string &);
	void processHeaders(const string &input);
public:
	HttpJailServer(string path="/"):URLPath(path),maxDataSize(MAXDATA),sizeToRead(MAXDATA){}
	uint32_t getClientIP(){return clientip;}
	string receive(int socket, time_t timeout, size_t sizelimit);
	enum CodeNumber {continueCode,
					 badRequestCode,
					 notFoundCode,
					 requestTimeoutCode,
					 requestEntityTooLargeCode,
					 internalServerErrorCode,
					 notImplementedCode};
	void send(int code, const string &codeText, const string &response);
	void sendRaw(const char * const, size_t);
	void send200(const string &);
	void sendCode(CodeNumber, string);
};

#endif
