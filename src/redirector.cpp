/**
 * @package:   Part of vpl-jail-system
 * @copyright: Copyright (C) 2013 Juan Carlos Rodríguez-del-Pino
 * @license:   GNU/GPL3, see LICENSE.txt or http://www.gnu.org/licenses/gpl-3.0.html
 **/

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <syslog.h>
#include <string.h>
#include <arpa/inet.h>
#include "jail_limits.h"
#include "redirector.h"

Redirector::Redirector(): bufferSizeLimit(50*1024) {
	state = error;
	timeout = 0; //Timeout when connecting
	noOutput = false; //true if program output nothing
}

/**
 * return if output buffer is full
 */
bool Redirector::isOutputBufferFull(){
	return (int) netbuf.size() >= bufferSizeLimit;
}

/**
 * Add string to network buffer
 * The string is read from program output
 * Cut string if size limit reached
 */
void Redirector::addOutput(const string &toAdd){
	if(toAdd.empty()) return;
	noOutput = false;
	//Control netbuf size limit
	if((int) (netbuf.size()+toAdd.size()) > 2*bufferSizeLimit){ //Buffer too large
		const char *text="\n=============== output cut to 1Mb =============\n";
		//take begin and end of netbuf+toAdd to truncate to 1MB
		string overflow=netbuf+toAdd;
		size_t ofsize=overflow.size();
		netbuf=overflow.substr(0,bufferSizeLimit/2)
						+text
						+overflow.substr(ofsize-bufferSizeLimit/2,bufferSizeLimit/2);
		Logger::log(LOG_INFO,"Program output has been cut to 1MB");
		static bool noLimited=true;
		if(noLimited){
			addMessage("\nJail: program output has been limited to 1MB\n");
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
	//if(events & POLLMSG) ret += "POLLMSG ";
	ret += ")";
	return ret;
}

/**
 * Advance for batch execution
 */
void RedirectorTerminalBatch::advance(){
	int oldstate = state;
	switch(state) {
	case begin:
		if (fdps < 0) {
			state = error; //fd pseudo terminal error
			break;
		}
		Util::fdblock(fdps, false);
		/* no break */
	case connecting:
		state = connected;
		/* no break */
	case connected:{
		//Poll to read from program
		struct pollfd devices[1];
		devices[0].fd = fdps;
		char buf[MAX];
		devices[0].events = POLLREAD; //removed |POLLOUT
		int res = poll(devices, 1, polltimeout);
		if (res == -1) { //Error
			state = error;
			break;
		}
		if(res == 0) break; //Nothing to do
		//Logger::log(LOG_INFO,"poll: program %d %s.",
		//			devices[0].revents,eventsToString(devices[0].revents).c_str());
		if (devices[0].revents & POLLREAD) { //Read program output
			int readsize = read(fdps, buf, MAX);
			if (readsize == 0) {
				Logger::log(LOG_INFO, "program output end: %m");
				state = end;
				break;		
			}
			if (readsize < 0) {
				Logger::log(LOG_INFO, "program output read error: %m");
				state = error;
				break; //program output read error
			}
			if (readsize >0) {
				netbuf += string(buf,readsize);
				break;
			}
		}
		if (devices[0].revents & POLLBAD) {
			Logger::log(LOG_INFO, "Program end or I/O error: %m %d %s.",
					devices[0].revents,eventsToString(devices[0].revents).c_str());
			state = error;
			break;
		}
		break;
	}
	case ending:
		state = end;
		/* no break */
	case end:
	case error:
		usleep(50000);
		break;
	}
	if(oldstate != state)
		Logger::log(LOG_INFO,"New redirector state %d => %d",oldstate,state);

}

/**
 * Advance for online execution
 */
void RedirectorTerminal::advance() {
	States oldstate=state;
	switch(state){
	case connecting:
		break;
	case begin:{
		if(fdps<0) {
			state=error; //fd pseudo terminal error
			break;
		}
		Util::fdblock(fdps,false);
		state=connected;
	case connected:{
		//Poll to write and read from program and net
		if(ws->isReadBuffered())
			programbuf += ws->receive();
		struct pollfd devices[2];
		devices[0].fd=fdps;
		devices[1].fd=ws->getSocket();
		char buf[MAX];
		if(programbuf.size()) devices[0].events=POLLREAD|POLLOUT;
		else devices[0].events=POLLREAD;
		devices[1].events=POLLREAD;
		int res=poll(devices,2,polltimeout);
		if(res==-1) { //Error
			Logger::log(LOG_INFO,"pool error %m");
			state = error;
			break;
		}
		if(res==0) break; //Nothing to do
		Logger::log(LOG_INFO,"poll: program %d %s",
				devices[0].revents,eventsToString(devices[0].revents).c_str());
		if(devices[1].revents & POLLREAD)
			programbuf += ws->receive();
		if((devices[0].revents & POLLREAD) && !isOutputBufferFull()){ //Read program output
			int readsize=read(fdps,buf,MAX);
			if(readsize <= 0){
				Logger::log(LOG_INFO,"program output read error: %m");
				state=ending;
				break; //program output read error
			}
			if(readsize >0) {
				ws->send(string(buf,readsize));
			}
		}
		if(programbuf.size()>0 && (devices[0].revents & POLLOUT)){ //Write to program
			int written=write(fdps,programbuf.data(),programbuf.size());
			if(written <=0) {
				Logger::log(LOG_INFO,"Write to program error: %m");
				state=ending;
				break;
			}
			programbuf.erase(0,written);
		}
		if((devices[0].revents & POLLBAD) && !(devices[0].revents & POLLREAD)){
			Logger::log(LOG_INFO,"Program end or I/O error: %m %d %s",devices[0].revents,eventsToString(devices[0].revents).c_str());
			state=ending;
			break;
		}
		break;
	}
	case ending:{
		if(isSilent()){
			Logger::log(LOG_INFO,"Program terminated with no output");
			ws->send("\nProgram terminated with no output\n");
		}
		if(messageBuf.size()>0){
			Logger::log(LOG_INFO,"Add jail message to output");
			ws->send(messageBuf);
			messageBuf="";
		}
	}
	state=end;
	/* no break */
	case end:
	case error:
		if(ws->isClosed())
			usleep(50000);
		else ws->close();
		break;
	}
	}
	if(oldstate != state)
		Logger::log(LOG_INFO,"New redirector state %d => %d",oldstate,state);
}

/**
 * Advance for VNC
 */
void RedirectorVNC::advance() {
	States oldstate=state;
	switch(state){
	case begin:{
		sock=socket(AF_INET,SOCK_STREAM,0);//0= IP protocol
		if(sock<0){
			state=error; //No socket available
			break;
		}
		int on=1;
		if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0 ) {
			Logger::log(LOG_ERR,"setsockopt(SO_REUSEADDR) failed: %m");
		}
		#ifdef SO_REUSEPORT
	    if (setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &on, sizeof(on)) < 0) {
			Logger::log(LOG_ERR,"setsockopt(SO_REUSEPORT) failed: %m");
	    }
		#endif
		//fdblock(sock,false);
		state=connecting;
	}
	/* no break */
	case connecting: {
		struct sockaddr_in sdir;
		memset(&sdir, 0, sizeof(sdir));
		sdir.sin_family = AF_INET;
		inet_pton(AF_INET,"127.0.0.1",&sdir.sin_addr);
		sdir.sin_port = htons( port );
		if (connect(sock, (const sockaddr*)&sdir, sizeof(sdir))==0) {
			//fdblock(sock,true);
			state = connected;
			break;
		}else{
			Logger::log(LOG_INFO, "socket connect to (127.0.0.1:%d) error: %m",(int)port);
			usleep(100000); // 1/10 seg
		}
		if (timeout < time(NULL)) {
			Logger::log(LOG_ERR, "socket connect timeout: %m");
			state = error;
		}
		break;
	}
	case connected:{
		//Poll to write and read from program and net
		if (ws->isClosed()) {
			Logger::log(LOG_INFO, "Websocket closed by client");
			state = end;
			break;
		}
		if (ws->isReadBuffered())
			netbuf += ws->receive(); //Read client data
		struct pollfd devices[2];
		devices[0].fd = sock;
		devices[1].fd = ws->getSocket();
		if (netbuf.size()) devices[0].events = POLLREAD | POLLOUT;
		else devices[0].events = POLLREAD;
		devices[1].events = POLLREAD;
		int res = poll(devices, 2, polltimeout);
		if (res == -1) { //Error
			Logger::log(LOG_INFO,"pool error %m");
			state = error;
			break;
		}
		if (res == 0) break; //Nothing to do
		Logger::log(LOG_INFO, "poll: server socket %d %s",
				devices[0].revents,eventsToString(devices[0].revents).c_str());
	    Logger::log(LOG_INFO, "poll: client socket %d %s",
				devices[1].revents,eventsToString(devices[1].revents).c_str());
		if (devices[1].revents & POLLREAD) { //Read vnc client data.
			netbuf += ws->receive();
		}
		if (devices[0].revents & POLLREAD) { //Read vncserver data.
			char buf[MAX];
			int readsize = read(sock, buf, MAX);
			if(readsize <= 0){ //Socket closed or error
				if(readsize < 0)
					Logger::log(LOG_INFO,"Receive from vncserver error: %m");
				else
					Logger::log(LOG_INFO,"Receive from vncserver size==0: %m");
				state = ending;
				break;
			}
			ws->send(string(buf, readsize), BINARY_FRAME);
		}
		if (netbuf.size()>0 && (devices[0].revents & POLLOUT)) { //Write to vncserver
			int written = write(sock, netbuf.data(), netbuf.size());
			if (written <= 0) { //close or error
				if ( written < 0)
					Logger::log(LOG_INFO,"Send to vncserver error: %m");
				state = ending;
				break;
			}
			netbuf.erase(0, written);
		}
		if ((devices[0].revents & POLLBAD) && !(devices[0].revents & POLLREAD)) {
			Logger::log(LOG_INFO,"Vncserver end or I/O error: %m %d %s",devices[0].revents,eventsToString(devices[0].revents).c_str());
			state=ending;
			break;
		}
		break;
	}
	case ending:
		state=end;
		/* no break */
	case end:
	case error:
		if (ws->isClosed()) {
			usleep(50000);
		} else {
			ws->close();
		}
		break;
	}
	if (oldstate != state)
		Logger::log(LOG_INFO,"New redirector state %d => %d",oldstate,state);
}

/**
 * Advance for Web
 */
void RedirectorWebServer::advance() {
	static string regexp = "(127\\.[0-9]+\\.[0-9]+\\.[0-9]+):([0-9]+)";
	static vplregex regServerAddress(regexp);
	States oldstate = state;
	switch(state) {
	case begin: {
		server = socket(AF_INET,SOCK_STREAM, 0); // 0= IP protocol
		if(server < 0){
			state=error; //No socket available
			break;
		}
		int on=1;
		if(setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0 ) {
			Logger::log(LOG_ERR,"setsockopt(SO_REUSEADDR) failed: %m");
		}
		#ifdef SO_REUSEPORT
	    if (setsockopt(server, SOL_SOCKET, SO_REUSEPORT, &on, sizeof(on)) < 0) {
			Logger::log(LOG_ERR,"setsockopt(SO_REUSEPORT) failed: %m");
	    }
		#endif
		state = connecting;
	}
	/* no break */
	case connecting: {
		struct sockaddr_in sdir;
		memset(&sdir, 0, sizeof(sdir));
		sdir.sin_family = AF_INET;
		{
			vplregmatch match(3);
			if ( ! regServerAddress.search(serverAddress, match) ) {
				Logger::log(LOG_ERR, "Bad local web server address %s", serverAddress.c_str());
				state = error;
				break;
			}
			string serverIP = match[1];
			int port = Util::atoi(match[2].c_str());
			inet_pton(AF_INET, match[1].c_str(), &sdir.sin_addr);
			sdir.sin_port = htons( port );
		}
		if( connect(server, (const sockaddr*) &sdir, sizeof(sdir)) == 0 ) {
			netbuf = client->getHeaders();
			state = connected;
			break;
		} else {
			Logger::log(LOG_INFO, "socket connecting to (%s) error: %m", serverAddress.c_str());
			usleep(100000); // 1/10 seg
		}
		if (timeout < time(NULL)) {
			Logger::log(LOG_ERR, "socket connect timeout: %m");
			state = error;
		}
		break;
	}
	case connected: {
		// Poll to write and read from program and net
		if(client->isClosed()){
			Logger::log(LOG_INFO, "Closed by client");
			state = end;
			break;
		}
		if( client->isReadBuffered() ) {
			netbuf += client->receive(); //Read client data
		}
		struct pollfd devices[2];
		devices[0].fd = server;
		devices[1].fd = client->getSocket();
		char buf[MAX];
		if (netbuf.size()) {
			devices[0].events = POLLREAD | POLLOUT;
		} else {
			devices[0].events = POLLREAD;
		}
		devices[1].events = POLLREAD;
		int res = poll(devices, 2, polltimeout);
		if ( res == -1 ) { //Error
			Logger::log(LOG_INFO, "pool error %m");
			state = error;
			break;
		}
		if ( res == 0 ) {
			break; //Nothing to do
		}
		Logger::log(LOG_INFO, "poll: server socket %d %s",
				devices[0].revents, eventsToString(devices[0].revents).c_str());
	    Logger::log(LOG_INFO, "poll: client socket %d %s",
				devices[1].revents, eventsToString(devices[1].revents).c_str());
		if (devices[1].revents & POLLREAD) { //Read message from client navigator
			netbuf += client->receive();
		}
		if ( devices[0].revents & POLLREAD ) { // Read vncserver data
			int readsize = read(server,buf,MAX);
			if(readsize <= 0){ //Socket closed or error
				if(readsize < 0)
					Logger::log(LOG_INFO,"Receive from vncserver error: %m");
				else
					Logger::log(LOG_INFO,"Receive from vncserver size==0: %m");
				state = ending;
				break;
			}
			client->send(string(buf, readsize));
		}
		if (netbuf.size() > 0 && (devices[0].revents & POLLOUT)) { //Write to local server
			int written = write(server, netbuf.data(), netbuf.size());
			if (written <= 0) { //close or error
				if (written < 0) {
					Logger::log(LOG_INFO,"Send to vncserver error: %m");
				}
				state = ending;
				break;
			}
			netbuf.erase(0, written);
		}
		if ((devices[0].revents & POLLBAD) && !(devices[0].revents & POLLREAD)) {
			Logger::log(LOG_INFO, "Local web server end or I/O error: %m %d %s",
			                 devices[0].revents, eventsToString(devices[0].revents).c_str());
			state = ending;
			break;
		}
		break;
	}
	case ending:
		state = end;
		/* no break */
	case end:
	case error:
		if(client->isClosed()) {
			usleep(50000);
		} else {
			client->close();
		}
		break;
	}
	if ( oldstate != state ) {
		Logger::log(LOG_INFO, "New web server redirector state %d => %d", oldstate, state);
	}
}
