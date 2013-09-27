/**
 * version:		$Id: redirector.h,v 1.2 2011-04-07 14:18:15 juanca Exp $
 * package:		Part of vpl-xmlrpc-jail
 * copyright:	Copyright (C) 2011 Juan Carlos Rodríguez-del-Pino. All rights reserved.
 * license:		GNU/GPL, see LICENSE.txt or http://www.gnu.org/licenses/gpl-2.0.html
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

class Redirector{
	int fdps;  //Pseudo terminal file descriptor
	int host;  //Host IP number
	int port;  //Port number at host
	string password; //Password to start connection
	int sock;    //Socket
	time_t timeout; //Timeout when connecting
	string messageBuf; //Buffer of messages from Jail system
	string programbuf; //Buffer from net to program
	string netbuf;  //Buffer from program to net
	const int bufferSizeLimit; //Size limit 50Kb

	enum States {begin, connecting, connected, ending, end, error} state;
	bool online;
	bool noOutput; //true if program output nothing
	void advanceOnline();
	void advanceBatch();
	static string eventsToString(int);
public:
	static void fdblock(int fd, bool set=true);
	Redirector():bufferSizeLimit(50*1024){state=error;}
	void start(const int fdps, const int host, const int port, const string &password);
	void start(const int fdps);
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
