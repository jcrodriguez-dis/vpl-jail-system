/**
 * version:		$Id: httpServer.cpp,v 1.6 2011-06-07 08:57:14 juanca Exp $
 * package:		Part of vpl-xmlrpc-jail
 * copyright:	Copyright (C) 2009 Juan Carlos Rodríguez-del-Pino. All rights reserved.
 * license:		GNU/GPL, see LICENSE.txt or http://www.gnu.org/licenses/gpl-2.0.html
 **/

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include "httpServer.h"
#include <syslog.h>
#include "util.h"
#include <cstdlib>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>
#include <sstream>

/**
 * Process the http request line
 * parse method, URL and version
 * @param line string to process
 * @param method
 * @param URL
 * @param version
 */
void HttpJailServer::parseRequestLine(string line,
										string &method, string &URL, string &version){
	method="";
	URL="";
	version="";
	size_t pos=line.find(" ",0);
	if(pos != string::npos){
		method=line.substr(0,pos);
		line.erase(0,pos+1);
	}
	pos=line.find(" ",0);
	if(pos != string::npos){
		URL=line.substr(0,pos);
		line.erase(0,pos+1);
	}
	version=line;
}

/**
 * Process http request headers (until CRLFCRLF)
 * Check request line and parse headers
 * @param input lines that form the request line and headers
 */
void HttpJailServer::processHeaders(const string &input){
	string line;
	size_t offset=0;
	while((line=Util::getLine(input,offset)).size()==0 && offset < input.size());
	string method,path,version;
	parseRequestLine(line,method,path,version);
	if(method != "POST"){
		syslog(LOG_ERR,"http method error: expected POST found %s",method.c_str());
		sendCode(notImplementedCode,"http GET request not implemented");
		throw "http GET request not implemented";
	}
	if(path != URLPath && !(URLPath=="/" && path=="")){
		syslog(LOG_ERR,"http URL path error: expected %s found %s",URLPath.c_str(),path.c_str());
		sendCode(notFoundCode,"http request URL path incorrect");
		throw "http request URL path not found";
	}
	if(version.size()< 6 || version.substr(0,6)!= "HTTP/1"){
		syslog(LOG_ERR,"http version error: found %s",version.c_str());
		sendCode(badRequestCode,"http version unsupported");
		throw "http version unsupported";
	}
	while((line=Util::getLine(input,offset)).size()){
		parseHeader(line);
	}
	if(headers.find("Content-Length") != headers.end()){
		sizeToRead = atoi(headers["Content-Length"].c_str());
		if(sizeToRead > maxDataSize){
			sendCode(requestEntityTooLargeCode,"http Request size too large");
			throw "http Request size too large";
		}
	}
	//TODO for expect response
/*	if(headers.find("Expect") != headers.end() && headers["Expect"]=="100-continue"){
		sendCode(100);
	}*/
}

/**
 * Process one header
 * Parse header and save it in header map
 * @param line header to process
 */
void HttpJailServer::parseHeader(const string &line){
	syslog(LOG_INFO,"Header :\"%s\"",line.c_str());
	size_t pos=line.find(": ");
	if(pos!= string::npos){
		headers[line.substr(0,pos)]= line.substr(pos+2,line.size()-(pos+2));
	}
}

/**
 * receive an http request
 * @param socket connected to client
 * @param timeout, read request time limit in seconds
 * @param sizelimit, request size limit in bytes
 */
string HttpJailServer::receive(int socket, time_t timeout, size_t sizelimit){
	struct sockaddr_in jail_client;
	socklen_t snlen=sizeof(jail_client);
	if(getpeername(socket,(struct sockaddr*)&jail_client,&snlen)){
		syslog(LOG_ERR,"getpeername fail %m");
		throw "getpeername fail";
	}
	this->clientip=jail_client.sin_addr.s_addr;
	this->maxDataSize=sizelimit;
	unsigned char *CIP=(unsigned char *)&(this->clientip);
	syslog(LOG_INFO,"Client: %d.%d.%d.%d",(int)CIP[0],(int)CIP[1],(int)CIP[2],(int)CIP[3]);
	this->socket=socket;
	bool headerRead=false;
   	struct pollfd devices[1];
   	devices[0].fd=socket;
	devices[0].events=POLLIN;
	string request;
	const int MAX=10*1024;
	char buf[MAX];
	const int wait=10000; // 10 milisec
	const int bad=POLLERR|POLLNVAL;
	time_t timeLimit=time(NULL)+timeout;
	while(true){
		int res=poll(devices,1,wait);
		if(res==-1) {
			syslog(LOG_INFO,"poll fail reading %m");
			throw "Error poll reading data"; //Error
		}
		if(time(NULL)>timeLimit){
			syslog(LOG_ERR,"http read timeout");
			sendCode(requestTimeoutCode,"http read timeout");
			throw "http read timeout";
		}
		if(res==0) continue; //Nothing to do
		if(devices[0].revents & POLLIN){ //Read from net
			int sizeRead=read(socket,buf,MAX);
			if(sizeRead >0) {
				request += string(buf,sizeRead);
				if(request.size()>sizeToRead){
					syslog(LOG_ERR,"http input data large than expected");
					sendCode(badRequestCode,"http input data large than expected");
					throw "http input data large than expected";
				}
				if(headerRead){
					if(maxDataSize==sizelimit && request.size()>15 && request.find("</methodCall>",request.size()-16) != string::npos){
						syslog(LOG_INFO,"Tag end of methodcall");
						break;
					}
				}else{
					size_t pos;
					if((pos=request.find("\r\n\r\n")) != string::npos){
						processHeaders(request.substr(0,pos+4));
						request.erase(0,pos+4);
						headerRead=true;
					}
				}
			}
			else if(sizeRead<0){
				syslog(LOG_ERR,"Error reading http data %m");
				throw "Error reading data";
			}
			else {
				syslog(LOG_INFO,"sizeRead==0");
				break;
			}
			if(request.size()==sizeToRead){
				syslog(LOG_INFO,"Content length read");
				break;
			}
		}
		if(devices[0].revents & POLLHUP){ //socket close
			syslog(LOG_INFO,"POLLHUP");
			break;
		}
		if(devices[0].revents & bad) {
			syslog(LOG_ERR,"Error reading http data %m");
			throw "Error reading data";
		}
   }
   return request;
}

/**
 * send raw data to client
 * @param s byte buffer to send
 * @param size of buffer
 */
void HttpJailServer::sendRaw(const char * const s, size_t size){
	syslog(LOG_INFO,"Sending %lu",(long unsigned int)size);
	size_t offset=0;
   	struct pollfd devices[1];
   	devices[0].fd=socket;
	devices[0].events=POLLOUT;
	const int wait=10; // 10 milisec
	const int bad=POLLERR|POLLHUP|POLLNVAL;
	while(true){
		int res=poll(devices,1,wait);
		if(res==-1) {
			syslog(LOG_ERR,"poll fail writing %m");
			throw "Error poll writing data"; //Error
		}
		if(res==0) continue; //Nothing to do
		if(devices[0].revents & POLLOUT){ //Write to net
			size_t toWrite=10*1024;
			if(toWrite>size-offset)
				toWrite=size-offset;
			int sizeWritten=write(socket,s+offset,toWrite);
			if(sizeWritten <=0) {
				syslog(LOG_ERR,"http write data error %m");
				throw "Error data size too large";
			}else{
				offset += sizeWritten;
			}
		}
		if(devices[0].revents & bad) {
			syslog(LOG_ERR,"Error writing http data %m");
			throw "Error writing data";
		}
		if(offset>=size) break;

   }
	syslog(LOG_INFO,"Send %lu",(long unsigned int)offset);
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
	output += "Server: vpl-xmlrpc-jail "+string(Util::version())+"\r\n";
	output += "Connection: close\r\n";
	output += "Content-Length: " + Util::itos(response.size()) + "\r\n";
	output += "Content-Type: text/xml;chartype=UTF-8\r\n\r\n";
	output += response;
	sendRaw(output.c_str(),output.size());
	if(code != 100){ //If code != CONTINUE
		close(socket);
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
	const char * codeText="";
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
		html="<html><body>"+string(codeText)+"</ br>"+text+"</body></html>";
	}
	send(cnumber,codeText,html);
}

