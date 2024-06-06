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
#include "xml.h"



/**
 * Process http request headers (until CRLFCRLF)
 * Check request line and parse headers
 * @param input lines that form the request line and headers
 */
void HttpJailServer::validateRequest(string expected_path){
	if(socket->getMethod() != "POST") {
		Logger::log(LOG_DEBUG, "http METHOD not implemented %s", socket->getMethod().c_str());
		throw HttpException(notImplementedCode,
				"http METHOD not implemented");
	}
	if(expected_path == "")
		expected_path="/";
	string URLPath = socket->getURLPath();
	if(URLPath == "")
		URLPath = "/";
	if(expected_path != URLPath){
		Logger::log(LOG_DEBUG, "http request URL path unexpected '%s'", URLPath.c_str());
		throw HttpException(notFoundCode, "http request URL path not found");
	}
}

string HttpJailServer::receive(){
	if (socket->getHeader("Expect") == "100-continue") {
		sendCode(continueCode);
	}
	int sizeToRead = atoi(socket->getHeader("Content-Length").c_str());
	if(sizeToRead != 0){
		if(sizeToRead > Configuration::getConfiguration()->getRequestMaxSize()){
			throw HttpException(requestEntityTooLargeCode
					,"http Request size too large"
					,socket->getHeader("Content-Length"));
		}
		string payload = socket->receive(sizeToRead);
		#ifdef LOGPAYLOAD
		Util::writeFile("/tmp/payload" + Util::itos(clock()) + ".txt", payload);
		#endif
		return payload;
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
 * Prepare a HTTP response
 * @param code http code
 * @param codeText http code text
 * @param load Payload to send
 * @param headMethod if HEAD method no payload send (optional)
 * @param extraHeader Extra headers to add (optional)
 * @return string HTTP format
 */
string HttpJailServer::prepare_HTTP(int code, const string &codeText, const string &load,
                          const bool headMethod, const string extraHeader) {
	string output;
	output +="HTTP/1.1 "+ Util::itos(code)+" "+codeText+"\r\n";
	if(code != 100){
		output += "Server: vpl-jail-system "+string(Util::version())+"\r\n";
		output += "Connection: close\r\n";
		if (extraHeader.length()) {
			output += extraHeader;
		}
		output += "Content-Length: " + Util::itos(load.size()) + "\r\n";
		int HSTSMaxAge = Configuration::getConfiguration()->getHSTSMaxAge();
		if (HSTSMaxAge >= 0) {
			output += "Strict-Transport-Security: max-age=" + Util::itos(HSTSMaxAge) + "\r\n";
		}
		Logger::log(LOG_DEBUG,"Response Content-Length: %lu",(unsigned long)load.size());
		output += "Content-Type: ";
		if(load.find("<?xml ") == 0) {
			output += "text/xml";
		} else if (load.find("{") == 0) {
			output += "application/json";
		} else if (load.find("<!DOCTYPE svg") == 0 || load.find("<svg") == 0) {
			output += "image/svg+xml";
		} else if (load.find("<!DOCTYPE html") == 0 || load.find("<html") == 0) {
			output += "text/html";
		} else {
			output += "text/plain";
		}
		output += "; charset=utf-8\r\n\r\n";
		if (! headMethod) {
			output += load;
		}
	}else{
		output += "\r\n";
	}
	return output;
}

/**
 * send a response to client
 * @param code http code
 * @param codeText http code text
 * @param load Payload to send
 * @param headMethod if HEAD method no payload send (optional)
 * @param extraHeader Extra headers to add (optional)
 */
void HttpJailServer::send(int code, const string &codeText, const string &load,
                          const bool headMethod, const string extraHeader) {
	Logger::log(LOG_DEBUG, "Sending http %d %s", (int)code, codeText.c_str());		
	sendRaw(prepare_HTTP(code, codeText, load, headMethod, extraHeader));
	if(code != 100){ //If code != CONTINUE
		socket->close();
	}
}

/**
 * Send a 200 OK response to client
 * 
 * @param response xml to send as content
 * @param headMethod if HEAD method no payload send (optional)
 * @param extraHeader Extra headers to add (optional)
 */
void HttpJailServer::send200(const string &response, const bool headMethod, const string extraHeader){
	send(200,"OK", response, headMethod, extraHeader);
}

/**
 * Send a response to client
 * 
 * @param code code to send
 * @param text Extra text explainning the code
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
	if (code != continueCode) {
		html = "<html><head><title>" + XML::encodeXML(codeText) + "</title></head><body>";
		html += "<h2>" + XML::encodeXML(codeText) + "</h2>";
		html += "<h3>" + XML::encodeXML(text) + "</h3>";
		html += "</body></html>";
	}
	send(cnumber, codeText, html);
}

