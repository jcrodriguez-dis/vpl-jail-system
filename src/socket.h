/**
 * version:		$Id: socket.h,v 1.10 2014-02-26 16:50:22 juanca Exp $
 * package:		Part of vpl-jail-system
 * copyright:	Copyright (C) 2014 Juan Carlos Rodríguez-del-Pino
 * license:		GNU/GPL, see LICENSE.txt or http://www.gnu.org/licenses/gpl-3.0.html
 **/

#ifndef SOCKET_H_INC
#define SOCKET_H_INC
#include <regex.h>
#include <string>
#include <map>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "configuration.h"

using namespace std;
class SSLBase{
	static SSLBase* singlenton;
	const SSL_METHOD *method;
	SSL_CTX *context;

	SSLBase(){
		const char *certFile="/etc/vpl/cert.pem";
		const char *keyFile="/etc/vpl/key.pem";
		SSL_library_init();
		OpenSSL_add_all_algorithms();
		SSL_load_error_strings(); 
		method = SSLv23_server_method();
		if(method == NULL){
			syslog(LOG_ERR,"SSLv23_server_method() fail: %s",ERR_reason_error_string(ERR_get_error()));
			exit(EXIT_FAILURE);
		}
		if((context = SSL_CTX_new((SSL_METHOD *)method)) == NULL){ //Conversion for backward compatibility
			syslog(LOG_ERR,"SSL_CTX_new() fail: %s",ERR_reason_error_string(ERR_get_error()));
			exit(EXIT_FAILURE);
		}
		if(SSL_CTX_use_certificate_file(context, certFile, SSL_FILETYPE_PEM) != 1){
			syslog(LOG_ERR,"SSL_CTX_use_certificate_file() fail: %s",ERR_reason_error_string(ERR_get_error()));
			exit(EXIT_FAILURE);
		}
		if(SSL_CTX_use_PrivateKey_file(context, keyFile, SSL_FILETYPE_PEM) != 1){
			syslog(LOG_ERR,"SSL_CTX_use_PrivateKey_file() fail: %s",ERR_reason_error_string(ERR_get_error()));
			exit(EXIT_FAILURE);
		}
		if ( !SSL_CTX_check_private_key(context) ){
			syslog(LOG_ERR,"SSL_CTX_check_private_key() fail: %s",ERR_reason_error_string(ERR_get_error()));
			exit(EXIT_FAILURE);
		}
	}
public:
	static SSLBase* getSSLBase(){
		if(singlenton == NULL) singlenton= new SSLBase();
		return singlenton;
	}
	SSL_CTX *getContext(){
		return context; 
	}
};

class Socket{
	int socket;			//open socket connect to client
	uint32_t clientip;	//client ip taken from socket
	string method;
	string version;
	string protocol;
	string URL;		//URL
	string URLPath;
	map<string,string> headers;		//Headers pair name:value
	size_t maxDataSize;	//Max program data size to read
	string header;
	string readBuffer;
	string writeBuffer;
	regex_t regRequestLine;
	regex_t regHeader;
	regex_t regURL;
	bool closed;
	void parseRequestLine(const string &line);
	void parseHeader(const string &line);
	void processHeaders(const string &input);
	virtual ssize_t netWrite(const void *, size_t );
	virtual ssize_t netRead(void *, size_t );
public:
	Socket(int socket);
	virtual ~Socket();
	void readHeaders();
	uint32_t getClientIP(){return clientip;}
	string getProtocol(){return protocol;}
	string getMethod(){return method;}
	string getVersion(){return version;}
	string getURLPath(){return URLPath;}
	string getHeader(string name);
	int getSocket(){ return socket;}
	bool isReadBuffered() { return readBuffer.size()>0;}
	bool isWriteBuffered() { return writeBuffer.size()>0;}
	bool isClosed(){return closed;}
	void close(){if(!closed) shutdown(socket,SHUT_WR);closed=true;}
	bool wait(const int msec=50); //Wait for a socket change until milisec time
	void send(const string &data, bool async=false);
	string receive(int sizeToReceive=0);
};

class SSLSocket: public Socket{
	SSL *ssl;
	virtual ssize_t netWrite(const void *b, size_t s){
		return SSL_write(ssl, b, s);
	}
	virtual ssize_t netRead(void *b, size_t s){
		return SSL_read(ssl, b, s);
	}
public:
	SSLSocket(int s): Socket(s){
		ssl = SSL_new(SSLBase::getSSLBase()->getContext());
		SSL_set_fd(ssl, s);
		SSL_accept(ssl);	
	}
	~SSLSocket(){
		SSL_free(ssl);
	}
};

#endif /* SOCKET_H_ */
