/**
 * package:		Part of vpl-jail-system
 * copyright:	Copyright (C) 2009 Juan Carlos Rodríguez-del-Pino. All rights reserved.
 * license:		GNU/GPL, see LICENSE.txt or http://www.gnu.org/licenses/gpl-3.0.html
 **/

/**
 * Deamon vpl-jail-system. jail for vpl using xmlrpc
 **/

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <memory>
#include <map>
#include <list>
#include <arpa/inet.h>
using namespace std;
#include "jail.h"
#include "util.h"

class Daemon{
	Configuration *configuration;
	static Daemon* singlenton;
	int port,sport;
	int listenSocket;
	int secureListenSocket;
	int actualSocket;
	int nSockets;
	struct pollfd sockets[2];
	bool isSecureSocket[2];
	string interface;
	//Access statistics
	struct {
		int requests;
		int errors;
		int rejected;
	} statistics;
	//Error log by IP
	struct Log{
		int requests;
		int errors;
		Log(){
			requests=0;
			errors=0;
		}
	};
	map<string,Log> logs;
	//Live tasks by pid
	struct Child{
		string IP;
		time_t start;
	};
	map<pid_t,Child> children;
	static bool finishRequest;
	static void SIGTERMHandler(int n){
		finishRequest=true;
	}

	/**
	 * Check jail security (/etc, paswords files, /home, etc)
	 */
	void checkJail(){
		struct stat info;

		string detc=configuration->getJailPath()+"/etc";
		if(lstat(detc.c_str(),&info))
			throw "jail /etc not checkable";
		if(info.st_uid !=0 || info.st_gid !=0)
			throw "jail /etc not owned by root";
		if(info.st_mode & 022)
			throw "Jail error: jail /etc with insecure permissions (group or others with write permission)";

		string fpasswd=configuration->getJailPath()+"/etc/passwd";
		if(lstat(fpasswd.c_str(),&info))
			throw "Jail error: jail /etc/passwd not checkable";
		if(info.st_uid !=0 || info.st_gid !=0)
			throw "Jail error: jail /etc/passwd not owned by root";
		if(info.st_mode & 033)
			throw "Jail error: jail /etc/passwd with insecure permissions (group or others with write permission)";

		string fgroup=configuration->getJailPath()+"/etc/group";
		if(lstat(fgroup.c_str(),&info))
			throw "Jail error: jail /etc/group not checkable";
		if(info.st_uid !=0 || info.st_gid !=0)
			throw "Jail error: jail /etc/group not owned by root";
		if(info.st_mode & 033)
			throw "Jail error: jail /etc/group with insecure permissions (group or others with write permission)";

		string fshadow=configuration->getJailPath()+"/etc/shadow";
		if(!lstat(fshadow.c_str(),&info)) {
			if(info.st_uid !=0 || info.st_gid !=0)
				throw "Jail error: jail /etc/shadow not owned by root";
			if(info.st_mode & 077)
				throw "Jail error: jail /etc/shadow with insecure permissions (group or others with some permission)";
		}
	}

	/**
	 * Check control directory
	 */
	void checkControlDir(){
		string cd=configuration->getControlPath();
		struct stat info;
		if(lstat(cd.c_str(),&info))
			throw "control path not checkable: "+cd;
		if(info.st_uid !=0 || info.st_gid !=0)
			throw "control path not owned by root: "+cd;
		if(info.st_mode & 022)
			throw "control path with insecure permissions (must be 0x44)";
	}

	//Return if the IP is banned
	bool isBanned(string IP){
		cleanOld(); /* Fixes by Guilherme Gomes */
		if ( configuration->getFail2Ban() == 0 || logs.find(IP) == logs.end()) {
			return false;
		} else {
			Log log=logs[IP];
			return log.errors > 20 * configuration->getFail2Ban() &&
				   log.errors > log.requests / 2;
		}
	}
	void countRequest(string IP, ExitStatus es){
		if(es == neutral){
			return;
		}
		Log log;
		if(logs.find(IP) != logs.end())
			log=logs[IP];
		log.requests++;
		if( es != success ){
			log.errors++;
			statistics.errors++;
		}
		logs[IP]=log;
	}
	void processChildEnd(pid_t pid, ExitStatus es){
		if(children.find(pid) == children.end())
			syslog(LOG_ERR,"Child end, but pid not found");
		Child c=children[pid];
		children.erase(pid);
		countRequest(c.IP,es);
	}
	//Check children finish and retrieve exit code
	void harvest(){
		while(true){
			if(children.size()==0) return;
			int status;
			pid_t wret=waitpid(-1, &status, WNOHANG);
			if(wret == 0) return; //All Children running o no child
			if(wret == -1){
				syslog(LOG_ERR,"Server waitpid error");
				return;
			}
			statistics.requests++;
			if(WIFSIGNALED(status)){
				processChildEnd(wret,internalError);
			}else if(WIFEXITED(status)){
				processChildEnd(wret, static_cast<ExitStatus>(WEXITSTATUS(status)));
			}
		}
	}
	//Reduce and/or remove log by time
	void cleanOld(){
		//Simple algorithm remove old every 5 minutes
		static time_t nextClear=0;
		if(nextClear > time(NULL)) return;
		nextClear= time(NULL)+5*60; //next 5 minutes
		logs.clear();
		/*
		for(map<string,Log>::iterator it=logs.begin(); it != logs.end(); it++){
			Log l=it->second;
			l.request /=2;
			l.errors  /=3;
			it->second = l;
		}
		 */
	}
	void launch(int listen, bool sec){
		struct sockaddr_in client;
		socklen_t slen = sizeof(client);
		actualSocket = ::accept(listen, (struct sockaddr *)&client, &slen);
		if (actualSocket < 0) {
			syslog(LOG_ERR,"accept() Error:%d  %m", actualSocket);
			return;
		}
		string IP;
		char dst[INET_ADDRSTRLEN];
		const char *d = inet_ntop(client.sin_family, &client.sin_addr, dst, INET_ADDRSTRLEN);
		if(d != NULL)
			IP=d;
		if (isBanned(IP)) {
			close(actualSocket);
			statistics.rejected++;
			countRequest(IP,internalError);
			syslog(LOG_ERR,"%s: Request rejected: IP banned", IP.c_str());
			return;
		}
		fcntl(actualSocket, FD_CLOEXEC); //Close socket on execve
		pid_t pid = fork();
		if (pid == 0) { //new process
			Socket *socket = NULL;
			close(listenSocket);
			if(sec)
				socket= new SSLSocket(actualSocket);
			else
				socket= new Socket(actualSocket);
			Jail jail(IP);
			jail.process(socket);
			delete socket;
			usleep(10000);//Wait for father process child
			_exit(EXIT_SUCCESS);
		}
		close(actualSocket);
		Child c;
		c.IP=IP;
		c.start=time(NULL);
		children[pid]=c;	
	}
	//Accept and launch son
	void accept(){
		int res=poll(sockets, nSockets, JAIL_ACCEPT_WAIT);
		if ( res == -1 ) {
			if (errno == EINTR) {
				return;
			}
			throw string("Error poll sockets in accept: ") + strerror(errno);
		}
		if (res==0) return; //No request
		for ( int port = 0; port < nSockets; port++ ) {
			if (sockets[port].revents & POLLIN) { // Any type http/https or ws/wss
				launch(sockets[port].fd, isSecureSocket[port]);
			}
		}
	}

	void initSocketServer(){
		this->port = configuration->getPort();
		this->sport = configuration->getSecurePort();
		this->interface = configuration->getInterface();
		this->listenSocket = -1;
		this->secureListenSocket = -1;
		this->nSockets = 0;
		struct sockaddr_in local;
		memset(&local, 0, sizeof(local));
		local.sin_family = AF_INET;
		if(this->interface>"")
			local.sin_addr.s_addr = inet_addr(interface.c_str());
		else
			local.sin_addr.s_addr = INADDR_ANY;
		if (this->port) {
			listenSocket = socket(AF_INET, SOCK_STREAM, 0);
			if (listenSocket == -1)
				throw "socket() error";
			#ifdef SO_REUSEPORT
			int on=1;
			if (setsockopt(listenSocket, SOL_SOCKET, SO_REUSEPORT, &on, sizeof(on)) < 0) {
				syslog(LOG_ERR,"setsockopt(SO_REUSEPORT) failed: %m");
			}
			#endif
			local.sin_port = htons(port);
			if (bind(listenSocket, (struct sockaddr *) &local, sizeof(local)) == -1)
				throw string("bind() error: ") + strerror(errno);
			if (listen(listenSocket, 100) == -1) //100 queue size
				throw "listen() error";
			syslog(LOG_INFO,"Listen at port %s:%d",inet_ntoa(local.sin_addr), port);
	    } else {
	    	syslog(LOG_INFO,"No plain http port used");
	    }

		if(this->sport){
			secureListenSocket = socket(AF_INET, SOCK_STREAM, 0);
			if (secureListenSocket == -1)
				throw "socket() error";
			#ifdef SO_REUSEPORT
			int on=1;
		    if (setsockopt(secureListenSocket, SOL_SOCKET, SO_REUSEPORT, &on, sizeof(on)) < 0) {
				syslog(LOG_ERR,"setsockopt(SO_REUSEPORT) failed: %m");
		    }
			#endif
			local.sin_port = htons(sport);
			if (bind(secureListenSocket, (struct sockaddr *) &local, sizeof(local)) == -1)
				throw "bind() secure port error";
			if (listen(secureListenSocket, 100) == -1) //100 queue size
				throw "listen() secure port error";
			syslog(LOG_INFO,"Listen at secure port %s:%d",inet_ntoa(local.sin_addr), sport);
		} else {
		   	syslog(LOG_INFO,"No https secure port used");
		}
		if ( !this->port && !this->sport ) {
			syslog(LOG_CRIT,"No PORT or SECURE_PORT defined");
			_exit(EXIT_FAILURE);
		}
		if (this->listenSocket >= 0) {
			sockets[nSockets].fd = this->listenSocket;
			sockets[nSockets].events = POLLIN;
			isSecureSocket[nSockets] = false;
			nSockets++;
		}
		if (this->sport >= 0) {
			sockets[nSockets].fd = this->secureListenSocket;
			sockets[nSockets].events = POLLIN;
			isSecureSocket[nSockets] = true;
			nSockets++;
		}
		actualSocket=-1;
	}
	void demonize(){
		pid_t child_pid = fork();
		if(child_pid < 0) {
			syslog(LOG_EMERG,"demonize() => fork() fail (child_pid < 0)");
			exit(EXIT_FAILURE);
		}
		if(child_pid > 0) _exit(EXIT_SUCCESS); //gradparent exit
		if(setsid() < 0) {
			syslog(LOG_EMERG,"demonize() => (setsid() < 0)");
			exit(EXIT_FAILURE);
		}
		pid_t grandchild_pid = fork();
		if(grandchild_pid < 0) {
			syslog(LOG_EMERG,"demonize() => fork() fail (grandchild_pid < 0)");
			exit(EXIT_FAILURE);
		}
		if(grandchild_pid > 0) _exit(EXIT_SUCCESS); //parent exit
		FILE *fd=fopen("/run/vpl-jail-server.pid","w");
		fprintf(fd,"%d",(int)getpid());
		fclose(fd);
	}
	Daemon(){
		signal(SIGPIPE,SIG_IGN);
		signal(SIGTERM,SIGTERMHandler);
		this->listenSocket=-1;
		this->secureListenSocket=-1;
		configuration=Configuration::getConfiguration();
		checkJail();
		checkControlDir();
		SSLBase::getSSLBase();
		initSocketServer();
		demonize();
	}
public:
	static Daemon* getDaemon(){
		if(singlenton == NULL) singlenton= new Daemon();
		return singlenton;
	}
	static void closeSockets(){
		Daemon* daemon=getDaemon();
		daemon->nSockets = 0;
		if(daemon->listenSocket>0){
			close(daemon->listenSocket);
			daemon->listenSocket=-1;
		}
		if(daemon->secureListenSocket>0){
			close(daemon->secureListenSocket);
			daemon->secureListenSocket=-1;
		}
		if(daemon->actualSocket>0){
			close(daemon->actualSocket);
			daemon->actualSocket=-1;
		}
	}
	void periodicTasks() {
		static int checkPoint = 5 * 60 * 1000 / JAIL_ACCEPT_WAIT; // 5 minutes.
		static int loops = 0;
		loops++;
		if ( loops >= checkPoint ) {
			loops = 0;
			int pid = fork();
			if (pid == 0) {
				try {
					SSLBase::getSSLBase()->updateCertificatesIfNeeded();
					processMonitor::cleanZombieTasks();
				} catch(...) {
				}
				_exit(0);
			}
			Child c;
			c.IP = "127.0.0.1";
			c.start = time(NULL);
			children[pid]=c;
		}
	}
	//Main loop: receive requests/dispatch and control child
	void loop(){ //FIXME implement e adequate exit
		while(!finishRequest){
			accept(); //Accept one request waiting JAIL_ACCEPT_WAIT msec
			harvest(); //Process all dead childrens
			periodicTasks();
		}
		closeSockets();
	}
};
