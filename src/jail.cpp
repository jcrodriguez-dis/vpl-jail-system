/**
 * @package:   Part of vpl-jail-system
 * @copyright: Copyright (C) 2014 Juan Carlos Rodríguez-del-Pino
 * @license:   GNU/GPL, see LICENSE.txt or http://www.gnu.org/licenses/gpl-3.0.html
 **/

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <climits>
#include <limits>
#include <cstring>
#include <algorithm>
#include <exception>
#include <sys/types.h>
#include <dirent.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pty.h>
#include "jail.h"
#include "redirector.h"
#include "httpServer.h"
#include "util.h"
#include "processMonitor.h"
#include "websocket.h"
/*
 * @return string "ready", "busy", "offline"
 */
string Jail::commandAvailable(int memRequested){
	syslog(LOG_INFO,"Memory requested %d",memRequested);
	if(Util::fileExists("/etc/nologin")){ //System going shutdown
		return "offline";
	}else {
		char *mem=(char*)malloc(memRequested);
		if(mem==NULL) return "busy";
		else free(mem);
	}
	return "ready";
}

void Jail::commandRequest(mapstruct &parsedata, string &adminticket,string &monitorticket,string &executionticket){
	syslog(LOG_INFO,"Request for process");
	processMonitor pm(adminticket,monitorticket,executionticket);
	pid_t pid=fork();
	if(pid==0){ //new process
		try {
			syslog(LOG_INFO,"parse files %lu",(long unsigned int)parsedata.size());
			mapstruct files=RPC::getFiles(parsedata["files"]);
			mapstruct filestodelete=RPC::getFiles(parsedata["filestodelete"]);
			mapstruct fileencoding;
			if ( parsedata.find("fileencoding") != parsedata.end() ) {
				fileencoding = RPC::getFiles(parsedata["fileencoding"]);
			}
			string script=parsedata["execute"]->getString();
			//Retrieve files to execution dir and options, decode data if needed
			for(mapstruct::iterator i=files.begin(); i!=files.end(); i++){
				string name=i->first;
				string data=i->second->getString();
				if ( fileencoding.find(name) != fileencoding.end()
					 && fileencoding[name]->getInt() == 1 ) {
					syslog(LOG_INFO,"Decoding file %s from b64",name.c_str());
					data = Base64::decode(data);
					if ( name.length() > 4 && name.substr(name.length() - 4, 4) == ".b64") {
						name = name.substr(0, name.length() - 4);
					}
				}
				syslog(LOG_INFO,"Write file %s data size %lu",name.c_str(),(long unsigned int)data.size());
				pm.writeFile(name,data);
			}
			ExecutionLimits executionLimits=Configuration::getConfiguration()->getLimits();
			syslog(LOG_INFO,"Reading parms");
			executionLimits.syslog("Config");
			executionLimits.maxtime=min(parsedata["maxtime"]->getInt(),executionLimits.maxtime);
			executionLimits.maxfilesize=min(parsedata["maxfilesize"]->getInt(),executionLimits.maxfilesize);
			executionLimits.maxmemory=min(parsedata["maxmemory"]->getInt(),executionLimits.maxmemory);
			executionLimits.maxprocesses=min(parsedata["maxprocesses"]->getInt(),executionLimits.maxprocesses);
			executionLimits.syslog("Request");
			string vpl_lang=parsedata["lang"]->getString();
			syslog(LOG_DEBUG,"VPL_LANG %s",vpl_lang.c_str());
			bool interactive=parsedata["interactive"]->getInt()>0;
			syslog(LOG_DEBUG,"interactive %d",parsedata["interactive"]->getInt());
			pm.setExtraInfo(executionLimits,interactive,vpl_lang);
			pm.setCompiler();
			syslog(LOG_INFO,"Compilation");
			string compilationOutput=run(pm,script);
			pm.setCompilationOutput(compilationOutput);
			bool compiled=pm.FileExists(VPL_EXECUTION)||pm.FileExists(VPL_WEXECUTION);
			if(compiled){
				//Delete files
				for(mapstruct::iterator i=filestodelete.begin(); i!=filestodelete.end(); i++){
					string name=i->first;
					syslog(LOG_INFO,"Delete file %s",name.c_str());
					pm.deleteFile(name);
				}
				if(!interactive && pm.FileExists(VPL_EXECUTION)){
					pm.setRunner();
					syslog(LOG_INFO,"Non interactive execution");
					string program;
					if(pm.installScript(".vpl_launcher.sh","vpl_batch_launcher.sh"))
						program=".vpl_launcher.sh";
					else
						program=VPL_EXECUTION;
					string executionOutput=run(pm,program);
					syslog(LOG_INFO,"Write execution result");
					pm.setExecutionOutput(executionOutput,true);
				}
			}else{
				syslog(LOG_INFO,"Compilation fail");
			}
		}
		catch(std::exception &e){
			syslog(LOG_ERR, "unexpected exception: %s %s:%d", e.what(), __FILE__, __LINE__);
			_exit(EXIT_FAILURE 	);
		}
		catch(string &e){
			syslog(LOG_ERR, "unexpected exception: %s %s:%d", e.c_str(), __FILE__, __LINE__);
			_exit(EXIT_FAILURE 	);
		}
		catch(HttpException &e){
			syslog(LOG_ERR, "unexpected exception: %s %s:%d", e.getLog().c_str(), __FILE__, __LINE__);
			_exit(EXIT_FAILURE 	);
		}
		catch(const char *e){
			syslog(LOG_ERR, "unexpected exception: %s %s:%d", e,__FILE__, __LINE__);
			_exit(EXIT_FAILURE 	);
		}
		catch(...){
			syslog(LOG_ERR, "unexpected exception %s:%d", __FILE__, __LINE__);
			_exit(EXIT_FAILURE 	);
		}
		_exit(EXIT_SUCCESS);
	}
}
void Jail::commandGetResult(string adminticket,string &compilation,string &execution, bool &executed, bool &interactive){
	processMonitor pm(adminticket);
	pm.getResult(compilation,execution,executed);
	interactive=pm.isInteractive();
}
bool Jail::commandRunning(string adminticket){
	try{
		processMonitor pm(adminticket);
		bool running = pm.isRunnig();
		if ( ! running ) {
			pm.cleanTask();
		}
		return running;
	}catch(...){
		return false;
	}
}
void Jail::commandStop(string adminticket){
	pid_t pid=fork();
	if(pid==0){ //new process
		try{
			processMonitor pm(adminticket);
			pm.cleanTask();
		}catch(...){

		}
		_exit(EXIT_SUCCESS);
	}
}
void Jail::commandMonitor(string monitorticket,Socket *s){
	processMonitor pm(monitorticket);
	webSocket ws(s);
	processState state=prestarting;
	time_t startTime = 0;
	time_t lastMessageTime = 0;
	time_t lastTime=pm.getStartTime();
	string lastMessage;
	while(state != stopped){
		processState newstate=pm.getState();
		time_t now = time(NULL);
		time_t timeout = pm.getStartTime() + pm.getMaxTime();
		if( ! lastMessage.empty() && now != lastMessageTime){
			ws.send(lastMessage+": "+Util::itos(now-startTime)+" seg");
			lastMessageTime = now;
		}
		if(newstate!=state){
			state=newstate;
			switch(state){
			case prestarting:
				break;
			case starting:
				syslog(LOG_DEBUG,"Monitor starting");
				startTime = now;
				lastMessageTime = now;
				lastMessage = "message:starting";
				ws.send(lastMessage);
				break;
			case compiling:
				syslog(LOG_DEBUG,"Monitor compiling");
				timeout=now+pm.getMaxTime();
				startTime = now;
				lastMessageTime = now;
				lastMessage = "message:compilation";
				ws.send(lastMessage);
				break;
			case beforeRunning:
				syslog(LOG_DEBUG,"Monitor beforeRunning");
				ws.send("compilation:"+pm.getCompilation());
				timeout=now+JAIL_SOCKET_TIMEOUT;
				if(pm.FileExists(VPL_EXECUTION)){
					syslog(LOG_DEBUG,"run:terminal");
					ws.send("run:terminal");
				}else if(pm.FileExists(VPL_WEXECUTION))
					ws.send("run:vnc:"+pm.getVNCPassword());
				else{
					ws.send("close:");
					ws.close();
					ws.wait(500); //wait client response
					pm.cleanTask();
					ws.receive();
					return;
				}
				break;
			case running:
				syslog(LOG_DEBUG,"Monitor running");
				startTime = now;
				timeout=now+pm.getMaxTime()+6 /* execution cleanup */;
				lastMessageTime = now;
				lastMessage = "message:running";
				ws.send(lastMessage);
				break;
			case retrieve:
				syslog(LOG_DEBUG,"Monitor retrieve");
				startTime = now;
				timeout=now+JAIL_HARVEST_TIMEOUT;
				ws.send("retrieve:");
				break;
			case stopped:
				syslog(LOG_DEBUG,"Monitor stopped");
				ws.send("close:");
				ws.close();
				ws.wait(500); //wait client response
				break;
			}
		}
		ws.wait(100); // 10 time a second
		
		string rec=ws.receive();
		if(ws.isClosed())
			break;
		if(rec.size()>0){ //Receive client close ws
			ws.close();
			break;
		}
		//Check running timeout
		if(state != starting && timeout< time(NULL)){
			ws.send("message:timeout");
			pm.cleanTask();
			usleep(3000000);
			ws.send("close:");
			ws.close();
			ws.wait(500); //wait client response
			ws.receive();
			return;
		}
		if(lastTime != now && pm.isOutOfMemory()){ //Every second check memory usage
			string ml= pm.getMemoryLimit();
			syslog(LOG_DEBUG,"Out of memory (%s)",ml.c_str());
			ws.send("message:outofmemory:"+ml);
			usleep(1500000);
			pm.cleanTask();
			usleep(1500000);
			ws.send("close:");
			ws.close();
			ws.wait(500); //wait client response
			ws.receive();
			return;
		}
		lastTime = now;
	}
	pm.cleanTask();
}

void Jail::commandExecute(string executeticket,Socket *s){
	processMonitor pm(executeticket);
	webSocket ws(s);
	if(pm.getSecurityLevel() != execute){
		syslog(LOG_ERR,"%s: Security. Try to execute request with no monitor ticket",IP.c_str());
		throw "Internal server error";
	}
	syslog(LOG_INFO,"Start executing");
	if(pm.getState() == beforeRunning){
		pm.setRunner();
		if(pm.FileExists(VPL_EXECUTION)){
			string program;
			if(pm.installScript(".vpl_launcher.sh","vpl_terminal_launcher.sh"))
				program=".vpl_launcher.sh";
			else
				program=VPL_EXECUTION;
			runTerminal(pm, ws, program);
		}
		else if(pm.FileExists(VPL_WEXECUTION)){
			if(pm.installScript(".vpl_launcher.sh","vpl_vnc_launcher.sh"))
				runVNC(pm, ws, ".vpl_launcher.sh");
			else
				syslog(LOG_ERR,"%s:Error: vpl_vnc_launcher.sh not installed",IP.c_str());
		}else{
			syslog(LOG_ERR,"%s:Error: no thing to run",IP.c_str());
		}
	}
}

bool Jail::isValidIPforRequest(){
	const vector<string> &dirs=configuration->getTaskOnlyFrom();
	if(dirs.size()==0) return true;
	int l = (int) dirs.size();
	for(int i = 0; i< l; i++){
		if(IP.find(dirs[i]) == 0)
			return true;
	}
	return false;
}


/**
 * Constructor, parameters => main parameters
 */
Jail::Jail(string IP){
	this->IP = IP;
	this->newpid = -1;
	this->redirectorpid = -1;
	configuration = Configuration::getConfiguration();
}

string Jail::predefinedURLResponse(string URLPath) {
	string page;
	if( Util::toUppercase(URLPath) == "/OK"){
		page = "<!DOCTYPE html><html><body>OK</body></html>";
		page += "<script>setTimeout(function(){window.close();},2000);</script>";
	} else if( URLPath == "/robots.txt"){
		page = "User-agent: *\nDisallow: /\n";
	} else if( URLPath == "/favicon.ico"){
		page = "<svg viewBox=\"0 0 25 14\" xmlns=\"http://www.w3.org/2000/svg\">"
				"<style> .logo { font: bold 12px Arial Rounded MT,Arial,Helvetica;"
				"fill: #277ab0; stroke: black; stroke-width: 0.7px;} </style>"
				"<text x=\"0\" y=\"12\" class=\"logo\">VPL</text></svg>";
	}
	return page;
}
/**
 * Process request
 *  3) read http request and header
 *  4) select http xmlrpc or websocket
 *  5) http: available, request, getresult, running, stop
 *  6) websocket: monitor, execute
 */
void Jail::process(Socket *socket){
	syslog(LOG_INFO,"Start server version %s",Util::version());
	string httpURLPath=configuration->getURLPath();
	HttpJailServer server(socket);
	try{
		socket->readHeaders();
		if(socket->headerSize()==0){
			if(socket->isSecure()){ //Don't count SSL error
				_exit(static_cast<int>(neutral));
			}else{
				_exit(EXIT_FAILURE);
			}
		}
		string httpMethod=socket->getMethod();
		if(Util::toUppercase(socket->getHeader("Upgrade"))!="WEBSOCKET"){
			syslog(LOG_INFO,"http(s) request");
			if(httpMethod=="GET"){
				string response = predefinedURLResponse(socket->getURLPath());
				if ( response.size() == 0 ) {
					throw HttpException(notFoundCode, "Http GET: Url path not found '" + socket->getURLPath() + "'");
				}
				server.send200(response);
				_exit(static_cast<int>(neutral));
			}
			if(httpMethod=="HEAD"){
				string response = predefinedURLResponse(socket->getURLPath());
				if ( response.size() == 0 ) {
					throw HttpException(notFoundCode, "Http HEAD: Url path not found '" + socket->getURLPath() + "'");
				}
				server.send200(response, true);
				_exit(static_cast<int>(neutral));
			}
			if(httpMethod != "POST"){
				throw HttpException(badRequestCode, "Http(s) Unsupported METHOD " + httpMethod);
			}
			if(!isValidIPforRequest()) {
				throw HttpException(badRequestCode, "Client not allowed");
			}
			server.validateRequest(httpURLPath);
			string data = server.receive();
			XML xml(data);
			string request = RPC::methodName(xml.getRoot());
			mapstruct parsedata = RPC::getData(xml.getRoot());
			syslog(LOG_INFO, "Execute request '%s'", request.c_str());
			if(request == "available"){
				ExecutionLimits jailLimits = configuration->getLimits();
				int memRequested = parsedata["maxmemory"]->getInt();
				string status = commandAvailable(memRequested);
				syslog(LOG_INFO,"Status: '%s'",status.c_str());
				server.send200(RPC::availableResponse(status,processMonitor::requestsInProgress(),
						jailLimits.maxtime,
						jailLimits.maxfilesize,
						jailLimits.maxmemory,
						jailLimits.maxprocesses,
						configuration->getSecurePort()));
			}else if(request == "request"){
				string adminticket,monitorticket,executionticket;
				commandRequest(parsedata, adminticket,monitorticket,executionticket);
				server.send200(RPC::requestResponse(adminticket,monitorticket,executionticket
						,configuration->getPort(),configuration->getSecurePort()));
			}else if(request == "getresult"){
				string adminticket,compilation,execution;
				bool executed,interactive;
				adminticket=parsedata["adminticket"]->getString();
				commandGetResult(adminticket, compilation, execution,executed,interactive);
				server.send200(RPC::getResultResponse(compilation,execution,executed,interactive));
			}else if(request == "running"){
				string adminticket;
				adminticket=parsedata["adminticket"]->getString();
				bool running = commandRunning(adminticket);
				if (! running && parsedata.count("pluginversion") == 0) {
					// Restores behaviour of < 2.3 version
					throw "Ticket invalid";
				}
				server.send200(RPC::runningResponse(commandRunning(adminticket)));
			}else if(request == "stop"){
				string adminticket;
				adminticket=parsedata["adminticket"]->getString();
				commandStop(adminticket);
				server.send200(RPC::stopResponse());
			}else{ //Error
				throw HttpException(badRequestCode,"Unknown request:"+request);
			}
		}else{ //Websocket
			if(socket->getMethod() != "GET"){
				throw "ws(s) Unsupported METHOD "+httpMethod;
			}
			string URLPath=socket->getURLPath();
			regex_t reg;
			regmatch_t match[3];
			regcomp(&reg, "^\\/([^\\/]+)\\/(.+)$", REG_EXTENDED);
			int nomatch=regexec(&reg, URLPath.c_str(),3, match, 0);
			if(nomatch)
				throw string("Bad URL");
			string ticket=URLPath.substr(match[1].rm_so,match[1].rm_eo-match[1].rm_so);
			string command=URLPath.substr(match[2].rm_so,match[2].rm_eo-match[2].rm_so);
			if(command == "monitor"){
				commandMonitor(ticket,socket);
			}else if(command == "execute"){
				commandExecute(ticket,socket);
			}else
				throw string("Bad command");
		}
		_exit(EXIT_SUCCESS);
	}
	catch(HttpException &exception){
		syslog(LOG_ERR,"%s:%s",IP.c_str(),exception.getLog().c_str());
		server.sendCode(exception.getCode(),exception.getMessage());
	}
	catch(std::exception &e){
		syslog(LOG_ERR,"%s:Unexpected exception %s on %s:%d",IP.c_str(), e.what(),__FILE__,__LINE__);
		server.sendCode(internalServerErrorCode,"Unknown error");
	}
	catch(string &exception){
		syslog(LOG_ERR,"%s:%s",IP.c_str(),exception.c_str());
		server.sendCode(internalServerErrorCode,exception);
	}
	catch(const char *s){
		syslog(LOG_ERR,"%s:%s",IP.c_str(),s);
		server.sendCode(internalServerErrorCode,s);
	}
	catch(...){
		syslog(LOG_ERR, "%s:Unexpected exception %s:%d",IP.c_str(),__FILE__,__LINE__);
		server.sendCode(internalServerErrorCode,"Unknown error");
	}
	_exit(EXIT_FAILURE);
}

/**
 * chdir and chroot to jail
 */
void Jail::goJail(){
	string jailPath=configuration->getJailPath();
	if(chdir(jailPath.c_str()) != 0)
		throw HttpException(internalServerErrorCode,"I can't chdir to jail",jailPath);
	if(chroot(jailPath.c_str()) != 0)
		throw HttpException(internalServerErrorCode,"I can't chroot to jail",jailPath);
	syslog(LOG_INFO,"chrooted \"%s\"",jailPath.c_str());
}

/**
 * Execute program at prisoner home directory
 */
void Jail::transferExecution(processMonitor &pm, string fileName){
	string dir=pm.getRelativeHomePath();
	syslog(LOG_DEBUG,"Jail::transferExecution to %s+%s",dir.c_str(),fileName.c_str());
	string fullname=dir+"/"+fileName;
	if(chdir(dir.c_str())){
		throw "I can't chdir to exec dir :"+dir;
	}
	if(!Util::fileExists(fullname)){
		throw string("transferExecution: execution file not found: ")+fullname;
	}
	char *arg[6];
	char *command= new char[fullname.size()+1];
	strcpy(command,fullname.c_str());
	setpgrp();
	int narg=0;
	arg[narg++] = command;
	arg[narg++] = NULL;
	int nenv=0;
	char *env[10];
	string uid=Util::itos(pm.getPrisonerID());
	string HOME="HOME=/home/p"+uid;
	env[nenv++]=(char *)HOME.c_str();
	string PATH="PATH="+configuration->getCleanPATH();
	env[nenv++]=(char *)PATH.c_str();
	env[nenv++]=(char *)"TERM=dumb";
	string UID="UID="+uid;
	env[nenv++]=(char *)UID.c_str();
	string USER="USER=p"+uid;
	env[nenv++]=(char *)USER.c_str();
	string USERNAME="USERNAME=Prisoner "+uid;
	env[nenv++]=(char *)USERNAME.c_str();
	string VPL_LANG="VPL_LANG="+pm.getLang();
	env[nenv++]=(char *)VPL_LANG.c_str();
	string VPL_VNCPASSWD="VPL_VNCPASSWD="+pm.getVNCPassword();
	if(pm.isInteractive()){
		env[nenv++]=(char *)VPL_VNCPASSWD.c_str();
	}
	env[nenv++]=NULL;
	syslog(LOG_DEBUG,"Running \"%s\"",command);
	execve(command,arg,env);
	throw string("I can't execve: ")+command+" ("+strerror(errno)+")";
}

/**
 * set user limits
 */
void Jail::setLimits(processMonitor &pm){
	ExecutionLimits executionLimits=pm.getLimits();
	executionLimits.syslog("setLimits");
	struct rlimit limit;
	limit.rlim_cur=0;
	limit.rlim_max=0;
	setrlimit(RLIMIT_CORE,&limit);
	limit.rlim_cur=executionLimits.maxtime;
	limit.rlim_max=executionLimits.maxtime;
	setrlimit(RLIMIT_CPU,&limit);
	if(executionLimits.maxfilesize>0){ //0 equals no limit
		limit.rlim_cur=executionLimits.maxfilesize;
		limit.rlim_max=executionLimits.maxfilesize;
		setrlimit(RLIMIT_FSIZE,&limit);
	}
	if(executionLimits.maxprocesses>0){ //0 equals no change
		limit.rlim_cur=executionLimits.maxprocesses;
		limit.rlim_max=executionLimits.maxprocesses;
		setrlimit(RLIMIT_NPROC,&limit);
	}
	//RLIMIT_MEMLOCK
	//RLIMIT_MSGQUEUE
	//RLIMIT_NICERLIMIT_NOFILE
	//RLIMIT_RTPRIO
	//RLIMIT_SIGPENDING
}

/**
 * run program controlling timeout and redirection
 */
string Jail::run(processMonitor &pm,string name, int othermaxtime){
	int maxtime;
	pm.getLimits().syslog("run");
	if(othermaxtime){
		maxtime=othermaxtime;
		syslog(LOG_INFO,"Other maxtime set: %d",othermaxtime);
	}
	else
		maxtime=pm.getMaxTime();
	int fdmaster=-1;
	signal(SIGTERM,SIG_IGN);
	signal(SIGKILL,SIG_IGN);
	newpid=forkpty(&fdmaster,NULL,NULL,NULL);
	if (newpid == -1) //fork error
		return "Jail: fork error";
	if(newpid == 0){ //new process
		try{
			goJail();
			setLimits(pm);
			pm.becomePrisoner();
			setsid();
			transferExecution(pm,name);
		}catch(const char *s){
			syslog(LOG_ERR,"Error running: %s",s);
			printf("\nJail error: %s\n",s);
		}
		catch(const string &s){
			syslog(LOG_ERR,"Error running: %s",s.c_str());
			printf("\nJail error: %s\n",s.c_str());
		}
		catch(...){
			syslog(LOG_ERR,"Error running");
			printf("\nJail error: at execution stage\n");
		}
		_exit(EXIT_SUCCESS);
	}
	syslog(LOG_INFO, "child pid %d",newpid);
	Redirector redirector;
	redirector.start(fdmaster);
	time_t startTime=time(NULL);
	time_t lastTime=startTime;
	int stopSignal=SIGTERM;
	bool noMonitor=false;
	int status;
	while(redirector.isActive()){
		redirector.advance();
		pid_t wret=waitpid(newpid, &status, WNOHANG);
		if(wret == newpid){
			if(WIFSIGNALED(status)){
				int signal = WTERMSIG(status);
				char buf[1000];
				sprintf(buf,"\r\nJail: program terminated due to \"%s\" (%d)",
						strsignal(signal),signal);
				redirector.addMessage(buf);
			}else if(WIFEXITED(status)){
				int exitcode = WEXITSTATUS(status);
				if(exitcode != EXIT_SUCCESS){
					char buf[100];
					sprintf(buf,"\r\nJail: program terminated normally with exit code %d.\n",exitcode);
					redirector.addMessage(buf);
				}
			}else{
				redirector.addMessage("\r\nJail: program terminated but unknown reason.");
			}
			newpid=-1;
			break;
		} else if(wret > 0){//waitpid error wret != newpid
			redirector.addMessage("\r\nJail waitpid error: ret>0.\n");
			break;
		} else if(wret == -1) { //waitpid error
			redirector.addMessage("\r\nJail waitpid error: ret==-1.\n");
			syslog(LOG_INFO,"Jail waitpid error: %m");
			break;
		}
		if (wret == 0){ //Process running
			time_t now=time(NULL);
			if(lastTime != now){
				int elapsedTime=now-startTime;
				lastTime = now;
				if(elapsedTime>JAIL_MONITORSTART_TIMEOUT && !pm.isMonitored()){
					if(stopSignal!=SIGKILL)
						redirector.addMessage("\r\nJail: browser connection error.\n");
					syslog(LOG_INFO,"Not monitored");
					kill(newpid,stopSignal);
					stopSignal=SIGKILL; //Second try
					noMonitor=true;
				}else if(elapsedTime > maxtime){
					redirector.addMessage("\r\nJail: execution time limit reached.\n");
					syslog(LOG_INFO,"Execution time limit (%d) reached"
							,maxtime);
					kill(newpid,stopSignal);
					stopSignal=SIGKILL; //Second try
				}
				else if(pm.isOutOfMemory()){
					string ml= pm.getMemoryLimit();
					if(stopSignal!=SIGKILL)
						redirector.addMessage("\r\nJail: out of memory ("+ml+")\n");
					syslog(LOG_INFO,"Out of memory (%s)",ml.c_str());
					kill(newpid,stopSignal);
					stopSignal=SIGKILL; //Second try
				}
			}
		}
	}
	//wait until 5sg for redirector to read and send program output
	for(int i=0;redirector.isActive() && i<50; i++){
		redirector.advance();
		usleep(100000); // 1/10 seg
	}
	string output=redirector.getOutput();
	syslog(LOG_DEBUG,"Complete program output: %s", output.c_str());
	if(noMonitor){
		pm.cleanTask();
	}
	return output;
}

/**
 * run program in terminal controlling timeout and redirection
 */
void Jail::runTerminal(processMonitor &pm, webSocket &ws, string name){
	int fdmaster=-1;
	ExecutionLimits executionLimits=pm.getLimits();
	signal(SIGTERM,SIG_IGN);
	signal(SIGKILL,SIG_IGN);
	newpid=forkpty(&fdmaster,NULL,NULL,NULL);
	if (newpid == -1){ //fork error
		syslog(LOG_INFO,"Jail: fork error %m");
		return;
	}
	if(newpid == 0){ //new process
		try{
			goJail();
			setLimits(pm);
			pm.becomePrisoner();
			setsid();
			transferExecution(pm,name);
		}catch(const char *s){
			syslog(LOG_ERR,"Error running terminal: %s",s);
		}
		catch(...){
			syslog(LOG_ERR,"Error running terminal");
		}
		_exit(EXIT_SUCCESS);
	}
	syslog(LOG_INFO, "child pid %d",newpid);
	Redirector redirector;
	syslog(LOG_INFO, "Redirector start terminal control");
	redirector.start(fdmaster,&ws);
	time_t startTime=time(NULL);
	time_t lastTime=startTime;
	bool noMonitor=false;
	int stopSignal=SIGTERM;
	int status;
	syslog(LOG_INFO,"run: start redirector loop");
	while(redirector.isActive() && !ws.isClosed()){
		redirector.advance();
		pid_t wret=waitpid(newpid, &status, WNOHANG);
		if(wret == 0){
			time_t now=time(NULL);
			if(lastTime != now){
				int elapsedTime=now-startTime;
				lastTime = now;
				if(elapsedTime>JAIL_MONITORSTART_TIMEOUT && !pm.isMonitored()){
					syslog(LOG_INFO,"Not monitored");
					noMonitor=true;
					if(stopSignal!=SIGKILL)
						redirector.addMessage("\r\nJail: process stopped\n");
					redirector.stop();
					kill(newpid,stopSignal);
					stopSignal=SIGKILL;
				}else if(elapsedTime > executionLimits.maxtime){
					if(stopSignal!=SIGKILL)
						redirector.addMessage("\r\nJail: execution time limit reached.\n");
					redirector.stop();
					syslog(LOG_INFO,"Execution time limit (%d) reached"
							,executionLimits.maxtime);
					kill(newpid,stopSignal);
					stopSignal=SIGKILL;
				}
				else if(pm.isOutOfMemory()){
					string ml= pm.getMemoryLimit();
					if(stopSignal!=SIGKILL)
						redirector.addMessage("\r\nJail: out of memory ("+ml+")\n");
					syslog(LOG_INFO,"Out of memory (%s)",ml.c_str());
					kill(newpid,stopSignal);
					stopSignal=SIGKILL; //Second try
				}
			}
		}else{ //Not running or error
			break;
		}
	}
	syslog(LOG_DEBUG,"End redirector loop");
	//wait until 5sg for redirector to read and send program output
	for(int i=0;redirector.isActive() && i<50; i++){
		redirector.advance();
		usleep(100000); // 1/10 seg
	}
	if(noMonitor){
		syslog(LOG_DEBUG,"Not monitored");
		pm.cleanTask();
	}
}

/**
 * run program in VNCserver controlling timeout and redirection
 */
void Jail::runVNC(processMonitor &pm, webSocket &ws, string name){
	ExecutionLimits executionLimits=pm.getLimits();
	signal(SIGTERM,SIG_IGN);
	signal(SIGKILL,SIG_IGN);
	string output=run(pm,name,10); //FIXME use constant
	syslog(LOG_DEBUG,"%s",output.c_str());
	int VNCServerPort=Util::atoi(output);
	Redirector redirector;
	syslog(LOG_INFO, "Redirector start vncserver control");
	redirector.start(&ws,VNCServerPort);
	time_t startTime=time(NULL);
	time_t lastTime=startTime;
	bool noMonitor=false;
	syslog(LOG_INFO,"run: start redirector loop");
	while(redirector.isActive() && !ws.isClosed()){
		redirector.advance();
		time_t now=time(NULL);
		if(lastTime != now){
			int elapsedTime=now-startTime;
			lastTime = now;
			//TODO report to user the out of resources
			if(elapsedTime > executionLimits.maxtime){
				syslog(LOG_INFO,"Execution time limit (%d) reached"
						,executionLimits.maxtime);
				redirector.stop();
				break;
			}
			if(pm.isOutOfMemory()){
				string ml= pm.getMemoryLimit();
				syslog(LOG_INFO,"Out of memory (%s)",ml.c_str());
				redirector.stop();
				break;
			}
			if(elapsedTime>JAIL_MONITORSTART_TIMEOUT && !pm.isMonitored()){
				syslog(LOG_INFO,"Not monitored");
				redirector.stop();
				noMonitor=true;
				break;
			}
		}
	}
	syslog(LOG_DEBUG,"End redirector loop");
	//wait until 5sg for redirector to read and send program output
	for(int i=0;redirector.isActive() && i<50; i++){
		redirector.advance();
		usleep(100000); // 1/10 seg
	}
	if(pm.installScript(".vpl_vnc_stopper.sh","vpl_vnc_stopper.sh")){
		output=run(pm,".vpl_vnc_stopper.sh",5); //FIXME use constant
		syslog(LOG_DEBUG,"%s",output.c_str());
	}
	if(noMonitor){
		pm.cleanTask();
	}
}
