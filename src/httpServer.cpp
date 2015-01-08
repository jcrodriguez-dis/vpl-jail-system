/**
 * package:		Part of vpl-jail-system
 * copyright:	Copyright (C) 2009 Juan Carlos Rodríguez-del-Pino
 * license:		GNU/GPL, see LICENSE.txt or http://www.gnu.org/licenses/gpl-3.0.html
 **/

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <syslog.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstdlib>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>
#include <sstream>
#include "util.h"
#include "httpServer.h"



/**
 * Process http request headers (until CRLFCRLF)
 * Check request line and parse headers
 * @param input lines that form the request line and headers
 */
void HttpJailServer::validateRequest(string path){
	if(socket->getMethod() != "POST"){
		throw HttpException(notImplementedCode,
				"http METHOD not implemented",socket->getMethod());
	}
	if(path == "")
		path="/";
	string URLPath=socket->getURLPath();
	if(URLPath == "")
		URLPath="/";
	if(path != URLPath){
		throw HttpException(notFoundCode
				,"http request URL path not found"
				, "unexpected path '"+path);
	}
}

string HttpJailServer::receive(){
	if(socket->getHeader("Expect")== "100-continue"){
		sendCode(continueCode);
	}
	int sizeToRead=atoi(socket->getHeader("Content-Length").c_str());
	if(sizeToRead != 0){
		if(sizeToRead > JAIL_REQUEST_MAX_SIZE){
			throw HttpException(requestEntityTooLargeCode
					,"http Request size too large"
					,socket->getHeader("Content-Length"));
		}
		return socket->receive(sizeToRead);
	}
	return "";
}

/**
 * send raw data to client
 * @param s byte buffer to send
 * @param size of buffer
 */
void HttpJailServer::sendRaw(const string &data){
	socket->send(data);
}

/**
 * send a response to client
 * @param int http code
 * @param string http code text
 * @param string response
 */
void HttpJailServer::send(int code, const string &codeText, const string &response){
	string output;
	output +="HTTP/1.1 "+ Util::itos(code)+" "+codeText+"\r\n";
	if(code != 100){
		output += "Server: vpl-jail-system "+string(Util::version())+"\r\n";
		output += "Connection: close\r\n";
		output += "Content-Length: " + Util::itos(response.size()) + "\r\n";
		syslog(LOG_DEBUG,"Response Content-Length: %lu",(unsigned long)response.size());
		output += "Content-Type: text/";
		if(code==200 && response.find("<!DOCTYPE html") != 0)
			output += "xml";
		else
			output += "html";
		output += ";chartype=UTF-8\r\n\r\n";
		output += response;
	}else{
		output += "\r\n";
	}
	sendRaw(output);
	if(code != 100){ //If code != CONTINUE
		socket->close();
	}
}

/**
 * send a 200 OK response to client
 * @param response xml to send as content
 */
void HttpJailServer::send200(const string &response){
	send(200,"OK", response);
}

/**
 * send a response to client
 * @param code code to send
 */
void HttpJailServer::sendCode(CodeNumber code, string text){
	string codeText;
	int cnumber=500;
	switch(code){
	case continueCode:
		cnumber=100;
		codeText="Continue";
		break;
	case badRequestCode:
		cnumber=400;
		codeText="Bad Request";
		break;
	case notFoundCode:
		cnumber=404;
		codeText="Not found";
		break;
	case methodNotAllowedCode:
		cnumber=405;
		codeText="Method not allowed";
		break;
	case requestTimeoutCode:
		cnumber=408;
		codeText="Request Time-out";
		break;
	case requestEntityTooLargeCode:
		cnumber=413;
		codeText="Request Entity Too Large";
		break;
	case internalServerErrorCode:
		cnumber=500;
		codeText="Internal Server Error";
		break;
	case notImplementedCode:
		cnumber=501;
		codeText="Not Implemented";
		break;
	};
	string html;
	if(code!=continueCode){
		html="<html><body><h2>"+codeText+"</h2><h3>"+text+"</h3></body></html>";
	}
	send(cnumber,codeText,html);
}

