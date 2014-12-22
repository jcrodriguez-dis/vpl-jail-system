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
	string interface;
	//Access statistics
	struct {
		int requests;
		int errors;
		int banned;
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
	static void catchSIGPIPE(int n){
		//Do nothing
	}

	/**
	 * Check jail security (/etc, paswords files, /home, etc)
	 */
	void checkJail(){
		string detc=configuration->getJailPath()+"/etc";
		struct stat info;
		if(lstat(detc.c_str(),&info))
			throw "jail /etc not checkable";
		if(info.st_uid !=0 || info.st_gid !=0)
			throw "jail /etc not owned by root";
		if(info.st_mode & 022)
			throw "Jail error: jail /etc with insecure permissions (must be 0xx44)";
		string fpasswd=configuration->getJailPath()+"/etc/passwd";
		if(lstat(fpasswd.c_str(),&info))
			throw "Jail error: jail /etc/passwd not checkable";
		if(info.st_uid !=0 || info.st_gid !=0)
			throw "Jail error: jail /etc/passwd not owned by root";
		if(info.st_mode & 033)
			throw "Jail error: jail /etc/passwd with insecure permissions (must be 0xx44)";

		string fshadow=configuration->getJailPath()+"/etc/shadow";
		if(!lstat(fshadow.c_str(),&info)) {
			if(info.st_uid !=0 || info.st_gid !=0)
				throw "Jail error: jail /etc/shadow not owned by root";
			if(info.st_mode & 077)
				throw "Jail error: jail /etc/passwd with insecure permissions (must be 0x00)";
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
		Log l;
		cleanOld(); /* Fix by Guilherme Gomes */
		if(logs.find(IP) != logs.end())
			l=logs[IP];
		bool banned=l.errors>10 && l.errors*2>l.requests;
		if(banned)
			statistics.banned++;
		return banned;
	}
	void endChild(pid_t pid, ExitStatus es){
		if(children.find(pid) == children.end())
			syslog(LOG_ERR,"Child end but not found");
		Child c=children[pid];
		children.erase(pid);
		Log l;
		if(logs.find(c.IP) != logs.end())
			l=logs[c.IP];
		l.requests++;
		if( es != success ){
			l.errors++;
			statistics.errors++;
		}
		logs[c.IP]=l;
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
				int signal = WTERMSIG(status);
				endChild(wret,internalError);
			}else if(WIFEXITED(status)){
				endChild(wret, static_cast<ExitStatus>(WEXITSTATUS(status)));
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
		socklen_t slen=sizeof(client);
		actualSocket = ::accept(listen,(struct sockaddr *)&client,&slen);
		if(actualSocket < 0)
			throw "Error accept";
		string IP;
		char dst[INET_ADDRSTRLEN];
		const char *d=inet_ntop(client.sin_family,&client.sin_addr,dst,INET_ADDRSTRLEN);
		if(d != NULL)
			IP=d;
		if(isBanned(IP)){
			syslog(LOG_ERR,"Request rejected: IP banned %s", IP.c_str());
			close(actualSocket);
			statistics.banned++;
			return;
		}
		pid_t pid=fork();
		Socket *socket;
		if(pid==0){ //new process
			Socket *socket=NULL;
			try{
				close(listenSocket);
				if(sec)
					socket= new SSLSocket(actualSocket);
				else
					socket= new Socket(actualSocket);
				Jail jail(IP);
				jail.process(socket);
			}
			catch(HttpException &exception){
				syslog(LOG_ERR,"%s",exception.getLog().c_str());
				if(socket ==NULL)
					socket = new Socket(actualSocket);
				HttpJailServer server(socket);
				server.sendCode(exception.getCode(),exception.getMessage());
				delete socket;
				exit(static_cast<int>(httpError));
			}
			catch(const char * const me){
				syslog(LOG_ERR,"%s",me);
				exit(static_cast<int>(internalError));
			}
			catch(...){
				syslog(LOG_ERR,"unexpected exception %s:%d",__FILE__,__LINE__);
				exit(static_cast<int>(internalError));
			}
			delete socket;
			exit(static_cast<int>(success));
		}
		close(actualSocket);
		Child c;
		c.IP=IP;
		c.start=time(NULL);
		children[pid]=c;	
	}
	//Accept and launch son
	void accept(){
		int ndev=1; 
		struct pollfd devices[2];
		devices[0].fd=listenSocket;
		devices[0].events=POLLIN;
		if(sport){
			devices[1].fd=secureListenSocket;
			devices[1].events=POLLIN;
			ndev++;
		}
		const int bad=POLLERR|POLLNVAL;
		int res=poll(devices,ndev,100);
		if(res==-1)
			throw "Error poll listener socket"; //Error
		if(res==0) return; //No request
		if(devices[0].revents & POLLIN){ //is http or ws
			launch(listenSocket,false);
		}
		if(devices[1].revents & POLLIN){ //is https or wss
			launch(secureListenSocket,true);
		}
	}

	void initSocketServer(){
		port = configuration->getPort();
		sport = configuration->getSecurePort();
		interface = configuration->getInterface();
		listenSocket = socket(AF_INET, SOCK_STREAM, 0);
		if (listenSocket == -1)
			throw "socket() error";
		struct sockaddr_in local;
		memset(&local, 0, sizeof(local));
		local.sin_family = AF_INET;
		if(interface>"")
			local.sin_addr.s_addr = inet_addr(interface.c_str());
		else
			local.sin_addr.s_addr = INADDR_ANY;
		local.sin_port = htons(port);
		if (bind(listenSocket, (struct sockaddr *) &local, sizeof(local)) == -1)
			throw "bind() error";
		if (listen(listenSocket, 100) == -1) //100 queue size
			throw "listen() error";
		syslog(LOG_INFO,"Listen at %s:%d",inet_ntoa(local.sin_addr), port);
		secureListenSocket = -1;
		if(sport){
			secureListenSocket = socket(AF_INET, SOCK_STREAM, 0);
			if (secureListenSocket == -1)
				throw "socket() error";
			local.sin_port = htons(sport);
			if (bind(secureListenSocket, (struct sockaddr *) &local, sizeof(local)) == -1)
				throw "bind() secure port error";
			if (listen(secureListenSocket, 100) == -1) //100 queue size
				throw "listen() secure port error";
			syslog(LOG_INFO,"Listen secure port at %s:%d",inet_ntoa(local.sin_addr), sport);
		}
		actualSocket=-1;
	}
	void demonize(){
		pid_t child_pid = fork();
		if(child_pid < 0) exit(EXIT_FAILURE);
		if(child_pid > 0) exit(EXIT_SUCCESS); //gradparent exit
		if(setsid() < 0) exit(EXIT_FAILURE);
		pid_t grandchild_pid = fork();
		if(grandchild_pid < 0) exit(EXIT_FAILURE);
		if(grandchild_pid > 0) exit(EXIT_SUCCESS); //parent exit
		FILE *fd=fopen("/var/run/vpl-jail-server.pid","w");
		fprintf(fd,"%d",(int)getpid());
		fclose(fd);
	}
	Daemon(){
		signal(SIGPIPE,SIG_IGN);
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
		close(daemon->listenSocket);
		if(daemon->secureListenSocket>0){
			close(daemon->secureListenSocket);
		}
		if(daemon->actualSocket>0){
			close(daemon->actualSocket);
		}
	}
	//Main loop: receive requests/dispatch and control child
	void loop(){ //FIXME implement e adecuate exit
		while(true){
			accept(); //Accept one request waiting 20 msec
			harvest(); //Process all dead childrens
			//TODO check SIG_TERM and then clean close 
		}
	}
};
