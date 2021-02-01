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
void HttpJailServer::validateRequest(string expected_path){
	if(socket->getMethod() != "POST"){
		throw HttpException(notImplementedCode,
				"http METHOD not implemented",socket->getMethod());
	}
	if(expected_path == "")
		expected_path="/";
	string URLPath=socket->getURLPath();
	if(URLPath == "")
		URLPath="/";
	if(expected_path != URLPath){
		throw HttpException(notFoundCode
				,"http request URL path not found"
				, "unexpected path '"+URLPath+"'");
	}
}

string HttpJailServer::receive(){
	if(socket->getHeader("Expect")== "100-continue"){
		sendCode(continueCode);
	}
	int sizeToRead=atoi(socket->getHeader("Content-Length").c_str());
	if(sizeToRead != 0){
		if(sizeToRead > Configuration::getConfiguration()->getRequestMaxSize()){
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
void HttpJailServer::send(int code, const string &codeText, const string &load, const bool headMethod){
	string output;
	output +="HTTP/1.1 "+ Util::itos(code)+" "+codeText+"\r\n";
	if(code != 100){
		output += "Server: vpl-jail-system "+string(Util::version())+"\r\n";
		output += "Connection: close\r\n";
		output += "Content-Length: " + Util::itos(load.size()) + "\r\n";
		syslog(LOG_DEBUG,"Response Content-Length: %lu",(unsigned long)load.size());
		output += "Content-Type: ";
		if(load.find("<?xml ") == 0) {
			output += "text/xml";
		} else if (load.find("<!DOCTYPE svg") == 0 || load.find("<svg") == 0) {
			output += "image/svg+xml";
		} else if (load.find("<!DOCTYPE html") == 0 || load.find("<html") == 0) {
			output += "text/html";
		} else {
			output += "text/plain";
		}
		output += "; chartype=UTF-8\r\n\r\n";
		if (! headMethod) {
			output += load;
		}
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
void HttpJailServer::send200(const string &response, const bool headMethod){
	send(200,"OK", response, headMethod);
}

/**
 * send a response to client
 * @param code code to send
 */
void HttpJailServer::sendCode(CodeNumber code, string text){
	string codeText;
	int cnumber = code;
	switch(code) {
	case continueCode:
		codeText = "Continue";
		break;
	case badRequestCode:
		codeText = "Bad Request";
		break;
	case notFoundCode:
		codeText = "Not found";
		break;
	case methodNotAllowedCode:
		codeText = "Method not allowed";
		break;
	case requestTimeoutCode:
		codeText = "Request Time-out";
		break;
	case requestEntityTooLargeCode:
		codeText = "Request Entity Too Large";
		break;
	case internalServerErrorCode:
		codeText = "Internal Server Error";
		break;
	case notImplementedCode:
		codeText = "Not Implemented";
		break;
	default:
		codeText = "Other internal Server Error";
		break;
	};
	string html;
	if(code!=continueCode){
		html = "<html><body><h2>"+codeText+"</h2><h3>"+text+"</h3></body></html>";
	}
	send(cnumber,codeText,html);
}

