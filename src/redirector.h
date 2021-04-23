/**
 * @package:   Part of vpl-jail-system
 * @copyright: Copyright (C) 2013 Juan Carlos Rodríguez-del-Pino
 * @license:   GNU/GPL3, see LICENSE.txt or http://www.gnu.org/licenses/gpl-3.0.html
 **/

#ifndef REDIRECTOR_INC_H
#define REDIRECTOR_INC_H
#include <string>
#include <fstream>
using namespace std;
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>
#include <signal.h>
#include <errno.h>
#include <pty.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include "websocket.h"

class Redirector {
protected:
	const int MAX = JAIL_NET_BUFFER_SIZE; //Buffer size to read
	const int POLLBAD = POLLERR | POLLHUP | POLLNVAL;
	const int POLLREAD = POLLIN | POLLPRI;
	const int polltimeout = 100; //  0.1 sec 
	time_t timeout; //Timeout when connecting
	string messageBuf; //Buffer of messages from Jail system
	string programbuf; //Buffer from net to program
	string netbuf;  //Buffer from program to net
	const int bufferSizeLimit; //Size limit 50Kb
	enum States {begin, connecting, connected, ending, end, error} state;
	bool noOutput; //true if program output nothing
	static string eventsToString(int);
public:
	Redirector();
	void stop() {state=ending;}
	virtual void advance() = 0;
	bool isError(){return state == error;}
	bool isActive(){return state != error && state != end;}
	bool isSilent(){return noOutput;}
	bool isOutputBufferFull();
	void addOutput(const string &);
	void addMessage(const string &);
	string getOutput();
	size_t getOutputSize();
};

class RedirectorTerminalBatch: public Redirector {
protected:
	int fdps;  //Pseudo terminal file descriptor
public:
	RedirectorTerminalBatch(const int fdps) {
		this->state = begin;
		this->fdps = fdps;
	};
	void advance();
};

class RedirectorTerminal: public Redirector {
protected:
	int fdps;  //Pseudo terminal file descriptor
	webSocket *ws; //webSocket for online
public:
	RedirectorTerminal(const int fdps, webSocket *s) {
		this->state = begin;
		this->fdps = fdps;
		this->ws = s;
	}
	void advance();
};

class RedirectorVNC: public Redirector {
protected:
	int port;  // VNC port number
	webSocket *ws; //webSocket for online
	int sock; // Connettion with VNC server
public:
	RedirectorVNC(webSocket *ws, const int port){
		this->state = begin;
		this->ws = ws;
		this->port = port;
		this->sock = -1;
		this->timeout = time(NULL) + 10;
	}
	void advance();
};

class RedirectorWebServer: public Redirector {
protected:
	Socket *client; // Socket object conneted with client
	string serverAddress; // Local web server address as 127.X.X.X:PORT
	int server; // Socket raw conneted with local web server
public:
	/**
	 * Constructor
	 * @param client Object connected with client navigator
	 * @param serverAddress Local web server address with format 127.X.X.X:PORT
	 */
	RedirectorWebServer(Socket *client, string serverAddress){
		this->state = begin;
		this->client = client;
		this->serverAddress = serverAddress;
		this->server = -1;
		this->timeout = time(NULL) + 10;
	}
	void advance();
};

#endif
