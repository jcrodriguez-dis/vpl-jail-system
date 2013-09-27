/**
 * version:		$Id: redirector.cpp,v 1.3 2011-04-07 14:26:47 juanca Exp $
 * package:		Part of vpl-xmlrpc-jail
 * copyright:	Copyright (C) 2009 Juan Carlos Rodríguez-del-Pino. All rights reserved.
 * license:		GNU/GPL, see LICENSE.txt or http://www.gnu.org/licenses/gpl-2.0.html
 **/

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <syslog.h>
#include <string.h>
#include "redirector.h"

/**
 * Set/Unset socket operation int block/nonblock mode
 */
void Redirector::fdblock(int fd, bool set){
	int flags;
	if( (flags = fcntl(fd, F_GETFL, 0)) < 0){
		syslog(LOG_ERR,"fcntl F_GETFL: %m");
	}
	if(set && (flags | O_NONBLOCK)==flags) flags ^=O_NONBLOCK;
	else flags |=O_NONBLOCK;
	if(fcntl(fd, F_SETFL, flags)<0){
		syslog(LOG_ERR,"fcntl F_SETFL: %m");
	}
}

/**
 * Start redirector (online connection)
 * fdps int: pseudo terminal file descriptor
 * host int: host IP
 * port int: port IP
 * password string: password to start host communication
 */
void Redirector::start(const int fdps, const int host, const int port, const string &password){
	this->state=begin;
	this->fdps=fdps;
	this->host=host;
	this->port=port;
	this->password=password;
	this->online=true;
	this->timeout=time(NULL)+5; //timeout in 5 seg
	this->netbuf="";
	this->programbuf="";
	this->noOutput = true;
}

/**
 * Start redirector (batch)
 * fdps int: pseudo terminal file descriptor
 */
void Redirector::start(const int fdps){
	this->state=begin;
	this->fdps=fdps;
	this->online=false;
	this->netbuf="";
	this->programbuf="";
	this->noOutput = true;
}

/**
 * Advance communication (unblocked operation)
 */
void Redirector::advance(){
	if(online){
		advanceOnline();
	}else{
		advanceBatch();
	}
}

/**
 * Advance for batch execution
 */
void Redirector::advanceBatch(){
	const int MAX=1024*10; //Buffer size to read
	const int POLLBAD=POLLERR|POLLHUP|POLLNVAL;
	const int POLLREAD=POLLIN|POLLPRI;
	switch(state){
		case begin:
			if(fdps<0) {
				state=error; //fd pseudo terminal error
				break;
			}
			fdblock(fdps,false);
		case connecting:
			state=connected;
		case connected:{
			//Poll to read from program
			struct pollfd devices[1];
			devices[0].fd=fdps;
			char buf[MAX];
			devices[0].events=POLLREAD|POLLOUT;
			int res=poll(devices,1,0);
			if(res==-1) { //Error
				state = error;
				break;
			}
			if(res==0) break; //Nothing to do
			syslog(LOG_INFO,"poll: program %d %s.",
						devices[0].revents,eventsToString(devices[0].revents).c_str());
			if(devices[0].revents & POLLREAD){ //Read program output
				int readsize=read(fdps,buf,MAX);
				if(readsize <= 0){
					syslog(LOG_INFO,"program output read error: %m");
					state=error;
					break; //program output read error
				}
				if(readsize >0) {
					netbuf += string(buf,readsize);
					break;
				}
			}
			if(devices[0].revents & POLLBAD){
				syslog(LOG_INFO,"Program end or I/O error: %m %d %s.",
						devices[0].revents,eventsToString(devices[0].revents).c_str());
				state=error;
				break;
			}
			break;
		}
		case ending:
			state=end;
		case end:
		case error:
			break;
	}
}

/**
 * Advance for online execution
 */
void Redirector::advanceOnline(){
	const int MAX=1024*10; //Buffer size to read
	const int POLLBAD=POLLERR|POLLHUP|POLLNVAL;
	const int POLLREAD=POLLIN|POLLPRI;
	switch(state){
		case begin:{
			if(fdps<0) {
				state=error; //fd pseudo terminal error
				break;
			}
			sock=socket(AF_INET,SOCK_STREAM,0);//0= IP protocol
			if(sock<0){
				state=error; //No socket available
				break;
			}
			int on=1;
			setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
			fdblock(sock,true);
			fdblock(fdps,false);
			state=connecting;
		}
		case connecting:{
			struct sockaddr_in sdir;
			memset(&sdir, 0, sizeof(sdir));
			sdir.sin_family = AF_INET;
			sdir.sin_addr.s_addr = host;
			sdir.sin_port = htons( port );
			if(connect(sock, (const sockaddr*)&sdir, sizeof(sdir))==0){
				fdblock(sock,false);
				netbuf=password;
				state=connected;
				break;
			}else{
				syslog(LOG_INFO, "socket connect to (%x:%d) error: %m", (int)host, (int)port);
			}
			if(timeout<time(NULL)){
				syslog(LOG_ERR, "socket connect timeout: %m");
				state=error;
			}
			break;
		}
		case connected:{
			//Poll to write and read from program and net
			struct pollfd devices[2];
			devices[0].fd=fdps;
			devices[1].fd=sock;
			char buf[MAX];
			if(programbuf.size()) devices[0].events=POLLREAD|POLLOUT;
			else devices[0].events=POLLREAD;
			if(netbuf.size()) devices[1].events=POLLREAD|POLLOUT;
			else devices[1].events=POLLREAD;
			int res=poll(devices,2,0);
			if(res==-1) { //Error
				syslog(LOG_INFO,"pool error %m");
				state = error;
				break;
			}
			if(res==0) break; //Nothing to do
			syslog(LOG_INFO,"poll: program %d %s. net %d %s",
						devices[0].revents,eventsToString(devices[0].revents).c_str(),
						devices[1].revents,eventsToString(devices[1].revents).c_str());

			if(devices[0].revents & POLLREAD && !isOutputBufferFull()){ //Read program output
				int readsize=read(fdps,buf,MAX);
				if(readsize <= 0){
					syslog(LOG_INFO,"program output read error: %m");
					state=ending;
					break; //program output read error
				}
				if(readsize >0) {
					addOutput(string(buf,readsize));
				}
			}
			if(devices[1].revents & POLLREAD){ //Read from net
				int readsize=read(sock,buf,MAX);
				if(readsize <= 0){
					syslog(LOG_INFO,"Socket closed at server end or error: %m");
					state=ending;
					break; //Socket closed at server end or error
				}
				if(readsize >0){
					programbuf += string(buf,readsize);
				}
			}
			if(programbuf.size()>0 && devices[0].revents & POLLOUT){ //Write to program
				int written=write(fdps,programbuf.c_str(),programbuf.size());
				if(written <=0) {
					syslog(LOG_INFO,"Write to program error: %m");
					state=ending;
					break;
				}
				programbuf.erase(0,written);
			}
			if(netbuf.size()>0 && devices[1].revents & POLLOUT){ //Write to net
				int written=write(sock,netbuf.c_str(),netbuf.size());
				if(written <=0){
					syslog(LOG_INFO,"Write to net error: %m");
					state=ending;
					break;
				}
				netbuf.erase(0,written);
			}
			if((devices[0].revents & POLLBAD) && !(devices[0].revents & POLLREAD)){
				syslog(LOG_INFO,"Program end or I/O error: %m %d %s",devices[0].revents,eventsToString(devices[0].revents).c_str());
				state=ending;
				break;
			}
			if(devices[1].revents & POLLBAD){
				syslog(LOG_INFO,"Net sock closed or I/O error: %m %d %s",devices[1].revents,eventsToString(devices[1].revents).c_str());
				state=error;
				break;
			}
			break;
		}
		case ending:{
			//Send buffered program output to net
			if(isSilent()){
				syslog(LOG_INFO,"Program terminated with no output");
				addOutput("\nProgram terminated with no output\n");
			}
			if(messageBuf.size()>0){
				syslog(LOG_INFO,"Add jail message to output");
				addOutput(messageBuf);
				messageBuf="";
			}
			if(netbuf.size()==0){ //Nothing to send
				syslog(LOG_INFO,"No more data to send");
				fdblock(sock,true);
				close(sock);
				state=end;
				break;
			}
			struct pollfd devices[1];
			devices[0].fd=sock;
			devices[0].events=POLLOUT;
			int res=poll(devices,1,0);
			if(res==-1) { //Error
				state = error;
				syslog(LOG_INFO,"pool error %m");
				break;
			}
			if(res==0){ //Nothing to do
				break;
			}
			if(devices[0].revents & POLLOUT){ //Write to net
				int written=write(sock,netbuf.c_str(),netbuf.size());
				if(written <=0) {
					syslog(LOG_INFO,"Write to net error: %m");
					state=error;
					break;
				}
				netbuf.erase(0,written);
				break;
			}
			if(devices[0].revents & POLLBAD && netbuf.size()==0){
				state=(devices[0].revents & POLLHUP)?end:error;
				break;
			}
		}
		case end:
		case error:
			break;
	}
}

/**
 * return if output buffer is full
 */
bool Redirector::isOutputBufferFull(){
	return netbuf.size()>=bufferSizeLimit;
}

/**
 * Add string to network buffer
 * The string is read from program output
 * Cut string if size limit reached
 */
void Redirector::addOutput(const string &toAdd){
	if(toAdd.size()==0) return;
	noOutput = false;
	//Control netbuf size limit
	if(netbuf.size()+toAdd.size()>2*bufferSizeLimit){ //Buffer too large
		const char *text="\n=============== output cut to 50Kb =============\n";
		//take begin and end of netbuf+toAdd to truncate to 50Kb
		string overflow=netbuf+toAdd;
		size_t ofsize=overflow.size();
		netbuf=overflow.substr(0,bufferSizeLimit/2)
				+text
				+overflow.substr(ofsize-bufferSizeLimit/2,bufferSizeLimit/2);
		syslog(LOG_INFO,"Program output has been cut to 50Kb");
		static bool noLimited=true;
		if(noLimited){
			addMessage("\nJail: program output has been limited to 50Kb\n");
			noLimited=false;
		}
	}else{
		netbuf += toAdd;
	}
}

/**
 * Add string to Message buffer
 * The string is jail information
 */
void Redirector::addMessage(const string &toAdd){
	messageBuf += toAdd;
}

/**
 * Return network buffer
 * Used when batch execution
 */
string Redirector::getOutput(){
	return netbuf;
}

/**
 * return network buffer size
 */
size_t Redirector::getOutputSize(){
	return netbuf.size();
}

/**
 * return pool events as text
 * Used when debugging
 */
string Redirector::eventsToString(int events){
	string ret="(";
	if(events & POLLIN) ret += "POLLIN ";
	if(events & POLLPRI) ret += "POLLPRI ";
	if(events & POLLOUT) ret += "POLLOUT ";
	if(events & POLLERR) ret += "POLLERR ";
	if(events & POLLHUP) ret += "POLLHUP ";
	if(events & POLLNVAL) ret += "POLLNVAL ";
	if(events & POLLRDNORM) ret += "POLLRDNORM ";
	if(events & POLLRDBAND) ret += "POLLRDBAND ";
	if(events & POLLWRNORM) ret += "POLLWRNORM ";
	if(events & POLLWRBAND) ret += "POLLWRBAND ";
	if(events & POLLMSG) ret += "POLLMSG ";
	ret += ")";
	return ret;
}
