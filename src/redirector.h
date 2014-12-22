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

class Redirector{
	int fdps;  //Pseudo terminal file descriptor
	int sock;    //Socket
	time_t timeout; //Timeout when connecting
	string messageBuf; //Buffer of messages from Jail system
	string programbuf; //Buffer from net to program
	string netbuf;  //Buffer from program to net
	const int bufferSizeLimit; //Size limit 50Kb
	int port; //Port of vncserver
	webSocket *ws; //webSocket for online
	enum States {begin, connecting, connected, ending, end, error} state;
	bool online;
	bool indirect;
	bool noOutput; //true if program output nothing
	void advanceOnline();
	void advanceIndirect();
	void advanceBatch();
	static string eventsToString(int);
public:
	Redirector();
	void start(webSocket *s, const int port);
	void start(const int fdps, webSocket *s);
	void start(const int fdps);
	void stop() {state=ending;}
	void advance();
	bool isError(){return state == error;}
	bool isActive(){return state != error && state != end;}
	bool isSilent(){return noOutput;}
	bool isOutputBufferFull();
	void addOutput(const string &);
	void addMessage(const string &);
	string getOutput();
	size_t getOutputSize();
};
#endif
