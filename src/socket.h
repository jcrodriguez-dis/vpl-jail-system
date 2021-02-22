/**
 * package:		Part of vpl-jail-system
 * copyright:	Copyright (C) 2014 Juan Carlos Rodríguez-del-Pino
 * license:		GNU/GPL, see LICENSE.txt or http://www.gnu.org/licenses/gpl-3.0.html
 **/

#ifndef SOCKET_H_INC
#define SOCKET_H_INC
#if HAVE_CONFIG_H
#include <config.h>
#endif
#include <regex.h>
#include <string>
#include <map>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/socket.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "util.h"
#include "configuration.h"

using namespace std;
class SSLBase{
	static SSLBase* singlenton;
	SSL_CTX *context;
	time_t timePrivateKeyFileModification;
	time_t timeCertificateFileModification;
	SSLBase(){
		this->context = NULL;
		this->timePrivateKeyFileModification = 0;
		this->timeCertificateFileModification = 0;
		SSL_library_init();
		OpenSSL_add_all_algorithms();
		SSL_load_error_strings();
		this->createContext();
	}

	void createContext() {
		Configuration* configuration = Configuration::getConfiguration();
		const string cipherList = configuration->getSSLCipherList();
		const string certFile = configuration->getSSLCertFile();
		const string keyFile = configuration->getSSLKeyFile();
		#ifdef HAVE_TLS_SERVER_METHOD
		const SSL_METHOD *method = TLS_server_method();
		if (method == NULL) {
			syslog(LOG_EMERG,"TLS_server_method() fail: %s",getError());
			_exit(EXIT_FAILURE);
		}
		#else
		const SSL_METHOD *method = SSLv23_server_method();
		if (method == NULL) {
			syslog(LOG_EMERG,"SSLv23_server_method() fail: %s",getError());
			_exit(EXIT_FAILURE);
		}
		#endif
		bool fail = false;
		SSL_CTX *newContext;
		if ((newContext = SSL_CTX_new((SSL_METHOD *)method)) == NULL) { //Conversion for backward compatibility
			syslog(LOG_EMERG,"SSL_CTX_new() fail: %s", getError());
			fail = true;
		} else if (SSL_CTX_use_certificate_chain_file(newContext, certFile.c_str()) != 1) {
			syslog(LOG_EMERG,"SSL_CTX_use_certificate_chain_file() fail: %s", getError());
			fail = true;
		} else if (SSL_CTX_use_PrivateKey_file(newContext, keyFile.c_str(), SSL_FILETYPE_PEM) != 1) {
			syslog(LOG_EMERG,"SSL_CTX_use_PrivateKey_file() fail: %s", getError());
			fail = true;
		} else if ( !SSL_CTX_check_private_key(newContext) ) {
			syslog(LOG_EMERG,"SSL_CTX_check_private_key() fail: %s", getError());
			fail = true;
		} else if ( cipherList[0] && SSL_CTX_set_cipher_list(newContext, cipherList.c_str()) == 0) {
			syslog(LOG_EMERG,"SSL_CTX_set_cipher_list() fail: %s", getError());
			fail = true;
		}
		if ( fail ) {
			if (this->context == NULL) {
				_exit(EXIT_FAILURE);
			} else {
				return;
			}
		}
		SSL_CTX_set_options(newContext,SSL_OP_NO_SSLv2|SSL_OP_NO_TLSv1_1|SSL_OP_NO_TLSv1);
		SSL_CTX_set_mode(newContext, SSL_MODE_ENABLE_PARTIAL_WRITE | SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);
		this->timePrivateKeyFileModification = Util::timeOfFileModification(keyFile);
		this->timeCertificateFileModification = Util::timeOfFileModification(certFile);
		if (this->context != NULL) {
			SSL_CTX_free(this->context);
		}
		this->context = newContext;
	}
public:
	static SSLBase* getSSLBase(){
		if(singlenton == NULL) singlenton= new SSLBase();
		return singlenton;
	}
	static const char *getError(){
		int error_n=ERR_get_error();
		const char* error_string="No detail";
		if(error_n !=0){
			error_string = ERR_error_string(error_n,NULL);
			if(error_string == NULL){
				error_string ="unregister error";
			}
		}
		return error_string;
	}
	SSL_CTX *getContext(){
		return context; 
	}
	void updateCertificatesIfNeeded() {
		Configuration* configuration = Configuration::getConfiguration();
		const string certFile = configuration->getSSLCertFile();
		const string keyFile = configuration->getSSLKeyFile();
		if ( this->timePrivateKeyFileModification != Util::timeOfFileModification(keyFile) ||
		     this->timeCertificateFileModification != Util::timeOfFileModification(certFile) ) {
			createContext();
		}
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
	size_t headerSize(){return header.size();}
	uint32_t getClientIP(){return clientip;}
	string getProtocol(){return protocol;}
	string getMethod(){return method;}
	string getVersion(){return version;}
	string getURLPath(){return URLPath;}
	string getHeader(string name);
	int getSocket(){ return socket;}
	bool isReadBuffered() { return readBuffer.size()>0;}
	bool isWriteBuffered() { return writeBuffer.size()>0;}
	virtual bool isSecure() { return false;}
	bool isClosed(){return closed;}
	void close(){if(!closed) shutdown(socket,SHUT_RDWR);closed=true;}
	bool wait(const int msec=50); //Wait for a socket change until milisec time
	void send(const string &data, bool async=false);
	string receive(int sizeToReceive=0);
};

class SSLRetry{
	struct pollfd devices[1];
	time_t currentTime,timeLimit;
	string message;
	const SSL *ssl;
public:
	SSLRetry(int socket, const SSL *s, string action){
		devices[0].fd=socket;
		ssl=s;
		message="Error in SSL "+action+" ";
		currentTime=time(NULL);
		timeLimit=currentTime+JAIL_SOCKET_TIMEOUT;
	}
	bool end(ssize_t ret){
		if(ret>0) return true;
		int code=SSL_get_error(ssl,ret);
		const char *scode=NULL;
		switch(code){
			case SSL_ERROR_NONE: return true;
			case SSL_ERROR_ZERO_RETURN: return true;
			case SSL_ERROR_WANT_READ:
				devices[0].events=POLLIN;
				break;
			case SSL_ERROR_WANT_WRITE:
				devices[0].events=POLLOUT;
				break;
			case SSL_ERROR_WANT_CONNECT:
				if(scode == NULL){
					scode="SSL_ERROR_WANT_CONNECT ";
				}
				/* no break */
			case SSL_ERROR_WANT_ACCEPT:
				if(scode == NULL){
					scode="SSL_ERROR_WANT_ACCEPT ";
				}
				/* no break */
			case SSL_ERROR_SYSCALL:
				if(scode == NULL){
					scode="SSL_ERROR_SYSCALL ";
				}
				if(ret == 0){
					syslog(LOG_INFO,"SSL socket closed unexpected ret==0: %s %s",message.c_str(),scode);
					return true;
				}
				/* no break */
			case SSL_ERROR_SSL:
				if(scode == NULL){
					scode="SSL_ERROR_SSL ";
				}
				/* no break */
			default:
				if(scode == NULL){
					scode="UNKNOW SSL_ERROR ";
				}
				throw HttpException(internalServerErrorCode
						,message+scode+SSLBase::getError()); //Error
		}
		ERR_clear_error();
		time_t wait=(timeLimit-currentTime)*1000;
		int res=poll(devices,1,wait);
		if(res==-1) {
			throw HttpException(internalServerErrorCode, message+ ": poll error"); //Error
		}
		currentTime=time(NULL);
		if(currentTime>timeLimit || res==0){
			throw HttpException(requestTimeoutCode, message+": timeout");
		}
		return false;
	}
};

class SSLSocket: public Socket{
	SSL *ssl;
	virtual ssize_t netWrite(const void *b, size_t s){
		SSLRetry retry(getSocket(),ssl,"write");
		while(true){
			ssize_t ret= SSL_write(ssl, b, s);
			if(retry.end(ret)) return ret;
		}
		return 0; //Not reachable
	}
	virtual ssize_t netRead(void *b, size_t s){
		SSLRetry retry(getSocket(),ssl,"read");
		while(true){
			ssize_t ret= SSL_read(ssl, b, s);
			if(retry.end(ret)) return ret;
		}
		return 0; //Not reachable
	}
public:
	SSLSocket(int s): Socket(s){
		ssl = SSL_new(SSLBase::getSSLBase()->getContext());
		Util::fdblock(s,false);
		SSL_set_fd(ssl, s);
		//SSL_accept with timeout
		SSLRetry retry(getSocket(),ssl,"accept");
		while(true){
			int ret= SSL_accept(ssl);
			if(retry.end(ret)) break;
		}
	}
	~SSLSocket(){
		SSL_free(ssl);
	}
	virtual bool isSecure() { return true;}
};

#endif /* SOCKET_H_ */
