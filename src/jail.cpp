/**
 * version:		$Id: jail.cpp,v 1.24 2011-06-07 08:58:16 juanca Exp $
 * package:		Part of vpl-xmlrpc-jail
 * copyright:	Copyright (C) 2011 Juan Carlos Rodríguez-del-Pino. All rights reserved.
 * license:		GNU/GPL, see LICENSE.txt or http://www.gnu.org/licenses/gpl-2.0.html
 **/

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include "jail.h"
#include "redirector.h"
#include "httpServer.h"
#include "util.h"
#include <climits>
#include <limits>
#include <cstring>
#include <algorithm>
#include <sys/types.h>
#include <dirent.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/**
 * Constructor, parameters => main parameters
 */
Jail::Jail(const int argcp, const char ** const argvp,char * const * const envp):argc(argcp),argv(argvp),environment(envp){
	needClean=false;
}

Jail::~Jail(){
	clean();
}

/**
 * Process request
 * 	1) Read configuration file "/etc/vpl-xmlrpc-jail.conf"
 * 	2) Check jail
 *  3) Select prisoner
 *  4) Read request
 *  5) Execute request
 */
void Jail::processRequest(){
	readConfigFile(); // Check and load configuration file
	checkJail();      // Check jail security
    selectPrisoner();
	syslog(LOG_INFO,"Create server version %s",Util::version());
	//Set URLPath from main arg if set "-urlpath PATH" or "/RPC"
	string URLPath=Util::getCommand(argc,argv,"-urlpath");
	if(URLPath==""){
		//For compatibility with ver <= 1.1
		URLPath = Util::getCommand(argc,argv,"-uri");
		if(URLPath == "")
			URLPath="/RPC";
	}
	HttpJailServer server(URLPath);
	syslog(LOG_INFO,"Read from net");
	//Receive request with http limits taken fron config file
	string data=server.receive(STDIN_FILENO, jailLimits.maxtime, jailLimits.maxmemory);
	XML xml(data);
	string request=RPC::methodName(xml.getRoot());
	syslog(LOG_INFO,"Write response");
	if(request=="status"){
		string status="ready";//TODO Test ready condition
        mapstruct parsedata=RPC::getData(xml.getRoot());
        int memRequested=parsedata["maxmemory"]->getInt();
		if(Util::fileExists("/etc/nologin")){ //System going shutdown
			status="offline";
		}else {
			char *mem=(char*)malloc(memRequested);
			if(mem==NULL){
				status="busy";
			}
			else{
				free(mem);
			}
		}
		server.send200(RPC::readyResponse(status,load(),
				jailLimits.maxtime,
				jailLimits.maxfilesize,
				jailLimits.maxmemory,
				jailLimits.maxprocesses,
				softwareInstalled));
		close(STDIN_FILENO);
	}
	else if(request=="execute"){
        syslog(LOG_INFO,"execute request");
        mapstruct parsedata=RPC::getData(xml.getRoot());
        syslog(LOG_INFO,"parse files %lu",(long unsigned int)parsedata.size());
        mapstruct files=RPC::getFiles(parsedata["files"]);
        mapstruct filestodelete=RPC::getFiles(parsedata["filestodelete"]);
        string script=parsedata["execute"]->getString();
        //Retrieve files to execution dir and options
        for(mapstruct::iterator i=files.begin(); i!=files.end(); i++){
			string name=i->first,data=i->second->getString();
			syslog(LOG_INFO,"Write file %s data size %lu",name.c_str(),(long unsigned int)data.size());
        	writeFile(name,data);
        }
		executionLimits=jailLimits;
		syslog(LOG_INFO,"Reading parms");
		executionLimits.maxtime=min(parsedata["maxtime"]->getInt(),executionLimits.maxtime);
		executionLimits.maxfilesize=min(parsedata["maxfilesize"]->getInt(),executionLimits.maxfilesize);
		executionLimits.maxmemory=min(parsedata["maxmemory"]->getInt(),executionLimits.maxmemory);
		executionLimits.maxprocesses=min(parsedata["maxprocesses"]->getInt(),executionLimits.maxprocesses);
		syslog(LOG_INFO,"maxtime %d",parsedata["maxtime"]->getInt());
		syslog(LOG_INFO,"maxfilesize %d",parsedata["maxfilesize"]->getInt());
		syslog(LOG_INFO,"maxmemory %d",parsedata["maxmemory"]->getInt());
		syslog(LOG_INFO,"maxprocesses %d",parsedata["maxprocesses"]->getInt());

		int interactive=parsedata["interactive"]->getInt();

		syslog(LOG_INFO,"Compilation");
        string outputCompilation=run(script);

        string outputExecution;
        bool compiled=Util::fileExists(prisonerHomePath()+"/vpl_execution");
        if(compiled){
        	//Delete files
	        for(mapstruct::iterator i=filestodelete.begin(); i!=filestodelete.end(); i++){
				string name=i->first;
				syslog(LOG_INFO,"Delete file %s",name.c_str());
	        	deleteFile(name);
	        }
	        if(interactive){
				syslog(LOG_INFO,"Interactive execution");
				int port=parsedata["port"]->getInt();
				string password=parsedata["password"]->getString();
				in_addr addr;
				int host;
				if(parsedata.find("ip") == parsedata.end() ||
					inet_aton(parsedata["ip"]->getString().c_str(),&addr) == 0){
					host = server.getClientIP();
				}else{
					host = addr.s_addr;
				}
				runInteractive("vpl_execution",host,port,password);
	        }else{
				syslog(LOG_INFO,"Non interactive execution");
	        	outputExecution=run("vpl_execution");
	        }
        }else{
       		syslog(LOG_INFO,"Compilation fail");
        }
		server.send200(RPC::executeResponse(outputCompilation,outputExecution,compiled));
		close(STDIN_FILENO);
	}
	else{
		server.sendCode(HttpJailServer::internalServerErrorCode,"Unknown request");
	}
}

/**
 * Prepare to run the execution interactive
 */
void Jail::runInteractive(string name, int hostip, int port, string password){
	interactive.requested=true;
	interactive.sname=name;
	interactive.host=hostip;
	interactive.port=port;
	interactive.password=password;
}

/**
 * Stop prisoner process if some one exists
 */
void Jail::stopPrisonerProcess(){
	syslog(LOG_INFO,"Sttoping prisoner process");
    pid_t pid=fork();
	if(pid==0){ //new process
	    changeUser();
	    //To not stop at first signal
	    signal(SIGTERM,Jail::catchSIGTERM);
		kill(-1,SIGTERM);
		usleep(100000);
		kill(-1,SIGKILL);
		abort();
	}
	else{
	   waitpid(pid,NULL,0); //Wait for pid end
	}
}

/**
 * return prisoner home page
 * if user root absolute path if prisoner relative to jail path
 */
string Jail::prisonerHomePath(){
	char buf[40];
	sprintf(buf,"/home/p%d",prisoner);
	uid_t uid=getuid();
	if(uid!=0){
		syslog(LOG_ERR,"prisonerHomePath %d",uid);
		return buf;
	}
	return jailPath+buf;
}

/**
 * remove a directory and its content
 * if force=true remove always
 * else remove files owned by prisoner
 * and complete directories owned by prisoner (all files and directories owns by prisoner or not)
 */
int Jail::removeDir(string dir, bool force){
	const string parent(".."),me(".");
	int nunlink=0;
	struct stat filestat;
	dirent *ent;
	DIR *dirfd=opendir(dir.c_str());
	if(dirfd==NULL){
		syslog(LOG_ERR,"Can't open dir \"%s\": %m",dir.c_str());
		return 0;
	}
	while((ent=readdir(dirfd))!=NULL){
		const string name(ent->d_name);
		const string fullname=dir+"/"+name;
		lstat(fullname.c_str(),&filestat);
		bool owned=filestat.st_uid == prisoner || filestat.st_gid == prisoner;
		if(ent->d_type & DT_DIR){
			if(name != parent && name != me){
				nunlink+=removeDir(fullname,force||owned);
			}
		}
		else{
			if(force || owned){
				if(unlink(fullname.c_str())){
					syslog(LOG_ERR,"Can't unlink \"%s\": %m",fullname.c_str());
				}
				else nunlink++;
			}
		}
	}
	closedir(dirfd);
	if(force){
		if(rmdir(dir.c_str())){
			syslog(LOG_ERR,"Can't rmdir \"%s\": %m",dir.c_str());
		}
	}
	return nunlink;
}

/**
 * remove prisoner home directory and /tmp prisoner files
 */
void Jail::removePrisonerHome(){
	syslog(LOG_INFO,"Remove prisoner home (%d)",prisoner);
	removeDir(prisonerHomePath(),true); //All files and dir
	removeDir(jailPath+"/tmp",false); //Only prisoner files and dirs
}

/**
 * Remove prisoner files and stop prisoner process
 */
void Jail::clean(){
	if(needClean){
		syslog(LOG_INFO,"Cleaning");
		stopPrisonerProcess();
		removePrisonerHome();
		needClean=false;
	}
}

/**
 * process request and run interactive if requested
 */
void Jail::process(){
	processRequest();
	if(interactive.requested){
		run(interactive.sname,interactive.host,
			interactive.port,interactive.password);
		interactive.requested=false;
	}
}

/**
 * Check configuration file security
 */
void Jail::checkConfig(){
	configPath="/etc/vpl/vpl-xmlrpc-jail.conf";
	struct stat info;
	if(lstat(configPath.c_str(),&info)) throw "Jail error: Config file not found";
	if(info.st_uid !=0 || info.st_gid !=0) throw "Jail error: config file not owned by root";
	if(info.st_mode & 077) throw "Jail error: config file with insecure permissions (must be 0600)";
}

/**
 * Check jail security (/etc, paswords files, /home, etc)
 */
void Jail::checkJail(){
	string detc=jailPath+"/etc";
	struct stat info;
	if(lstat(detc.c_str(),&info)) throw "Jail error: jail /etc not checkable";
	if(info.st_uid !=0 || info.st_gid !=0) throw "Jail error: jail /etc not owned by root";
	if(info.st_mode & 022) throw "Jail error: jail /etc with insecure permissions (must be 0xx44)";

	string fpasswd=jailPath+"/etc/passwd";
	if(lstat(fpasswd.c_str(),&info)) throw "Jail error: jail /etc/passwd not checkable";
	if(info.st_uid !=0 || info.st_gid !=0) throw "Jail error: jail /etc/passwd not owned by root";
	if(info.st_mode & 033) throw "Jail error: jail /etc/passwd with insecure permissions (must be 0xx44)";

	string fshadow=jailPath+"/etc/shadow";
	if(!lstat(fshadow.c_str(),&info)) {
		if(info.st_uid !=0 || info.st_gid !=0) throw "Jail error: jail /etc/shadow not owned by root";
		if(info.st_mode & 077) throw "Jail error: jail /etc/passwd with insecure permissions (must be 0x00)";
	}
}

/**
 * Check password for dangerous chars
 */
void Jail::checkPath(const string c,const int minSize){
	if((int)c.size() < minSize) throw "Jail error: Path too short";
	if(c.find("..") != string::npos) throw "Jail error: path with forbidden chars";
}

/**
 * Parse configuration file line
 * @param line line to parse
 * @param param string found before "="
 * @param svalue string after "="
 * @param value int value after "="
 * @return true if well constructed
 */
bool Jail::parseConfigLine(const string &line, string &param, string &svalue, int &value){
	string aux=line;
	param="";
	svalue="";
	value=0;
	int l=aux.size();
	for(int i=0; i< l; i++){ //remove comment
		if(aux[i]=='#'){
	  		aux.erase(i,l-i);
	  		break;
		}
	}
	Util::trim(aux);  //Clean string
	l=aux.size();
	for(int i=0; i< l; i++){
		if(aux[i]=='='){ //Locate '='
			param=aux.substr(0,i);
			Util::trim(param);
			svalue=aux.substr(i+1,l-(i+1));
			Util::trim(svalue);
	  		value=atoi(svalue.c_str());
	  		return true;
		}
	}
	return false;
}

/**
 * Read and parse configuration file
 */
void Jail::readConfigFile(){
	jailLimits.maxtime=INT_MAX;
	jailLimits.maxfilesize=INT_MAX;
	jailLimits.maxmemory=INT_MAX;
	jailLimits.maxprocesses=INT_MAX;
	checkConfig();
	ifstream file;
	file.open(configPath.c_str(),ifstream::in);
	if(file.fail())
   		throw "Jail error: config file can't be opened";
	int nl=0;
	while(!file.fail()){
		string line, param, svalue;
		int value;
		getline(file,line);
		if(file.fail()) break;
		nl++;
		if(parseConfigLine(line, param, svalue, value)){
			if(param=="MIN_PRISONER_UGID"){
				syslog(LOG_INFO,"MIN_PRISONER_UGID %d",value);
				if(value>1000) minPrisoner=value;
				else throw "Jail error: incorrect MIN_PRISONER_UGID value";
			}
			else if(param=="MAX_PRISONER_UGID"){
				syslog(LOG_INFO,"MAX_PRISONER_UGID %d",value);
				if(value<=65534) maxPrisoner=value;
				else throw "Jail error: incorrect MAX_PRISONER_UGID value";
			}
			else if(param=="JAILPATH" ){
				syslog(LOG_INFO,"JAILPATH %s",svalue.c_str());
				while(svalue.size()>0 && svalue[svalue.size()-1] == '/')
					svalue.erase(svalue.size()-1,1);
				checkPath(svalue);
				jailPath=svalue;
			}
			else if(param=="MAXTIME" ){
				syslog(LOG_INFO,"MAXTIME %d",value);
				jailLimits.maxtime=value;
			}
			else if(param=="MAXFILESIZE" ){
				syslog(LOG_INFO,"MAXFILESIZE %d",value);
				jailLimits.maxfilesize=value;
			}
			else if(param=="MAXMEMORY" ){
				syslog(LOG_INFO,"MAXMEMORY %d",value);
				jailLimits.maxmemory=value;
			}
			else if(param=="MAXPROCESSES" ){
				syslog(LOG_INFO,"MAXPROCESSES %d",value);
				jailLimits.maxprocesses=value;
			}
			else if(param=="SOFTINSTALLED" ){
				syslog(LOG_INFO,"SOFTINSTALLED \"%s\"",svalue.c_str());
				softwareInstalled+=";"+svalue;
			}
			else if(param != ""){ //Error
           		static char buf[100];
           		sprintf(buf,"Jail error: incorrect config file (line %d)",nl);
           		throw buf;
			}
		}
	}
	if(jailPath.size()==0) throw "Jail error: incorrect config file, JAILPATH no set";
	if(minPrisoner>maxPrisoner || minPrisoner<1000 || maxPrisoner>65534)
  		throw "Jail error: incorrect config file, prisoner uid bad defined";
}

/**
 * Return number of prisoner in jail
 */
int Jail::load(){
	const string homeDir=jailPath+"/home";
	const string pre="p";
	int nprisoner;
	dirent *ent;
	DIR *dirfd=opendir(homeDir.c_str());
	if(dirfd==NULL){
		syslog(LOG_ERR,"Can't open dir \"%s\": %m",homeDir.c_str());
		return 0;
	}
	while((ent=readdir(dirfd))!=NULL){
		const string name(ent->d_name);
		if(name.find(pre,0)==0) nprisoner++;
	}
	closedir(dirfd);
	return nprisoner-1; //Remove own prisoner
}

/**
 * Select for a prisoner id form minPrisoner to maxPrisoner
 */
void Jail::selectPrisoner(){
	int const range=(maxPrisoner-minPrisoner)+1;
	int const ntry=range*2;
	int const umask=0700;
	srand(time(NULL)%RAND_MAX);
	for(int i=0; i<ntry; i++){
		prisoner = minPrisoner+rand()%range;
		string homePath=prisonerHomePath();
		if(mkdir(homePath.c_str(),umask)==0){
			needClean=true;
			if(chown(homePath.c_str(),prisoner,prisoner)){
				syslog(LOG_ERR,"can't change prisoner home dir owner: %m");
				throw "Jail error: can't change prisoner home dir owner";
			}
			return;
		}
	}
	throw "Jail error: can't create prisoner home dir";
}

/**
 * Write a file to prisoner home
 */
void Jail::writeFile(string name, string data){
	checkPath(name,1);
	string fullName=prisonerHomePath()+"/"+name;
	FILE *fd=fopen(fullName.c_str(),"wb");
	if(fd==NULL) throw "Jail error: can't write file";
	bool isScript=name.size()>4 && name.substr(name.size()-3,3) == ".sh";
	if(isScript){ //Endline converted to UNIX
		string newdata;
		for(size_t i=0; i<data.size(); i++){
			if(data[i] != '\r') newdata+=data[i];
			else{
				char p=' ',n=' ';
				if(i>0) p=data[i-1];
				if(i+1<data.size()) n=data[i+1];
				if(p != '\n' && n != '\n') newdata += '\n';
			}
		}
		data=newdata;
	}
	if(data.size()>0 && fwrite(data.c_str(),data.size(),1,fd)!=1) throw "Jail error: can't write to file";
	fclose(fd);
	if(chown(fullName.c_str(),prisoner,prisoner)) syslog(LOG_ERR,"Can't change file owner %m");
	if(chmod(fullName.c_str(),isScript?0700:0600)) syslog(LOG_ERR,"Can't change file perm %m");
}

/**
 * Read a file from prisoner home directory
 */
string Jail::readFile(string name){
	checkPath(name,1);
	string fullName=prisonerHomePath()+"/"+name;
	FILE *fd=fopen(fullName.c_str(),"rb");
	if(fd==NULL) throw "Jail error: can't read file";
	string res;
	const int sbuffer=1024;
	char buffer[sbuffer];
	size_t read;
	while((read=fread(buffer,1,sbuffer,fd))>0){
        res+=string(buffer,read);
    }
    fclose(fd);
	return res;
}

/**
 * Read a file from prisoner home directory
 */
void Jail::deleteFile(string name){
	checkPath(name,1);
	string fullName=prisonerHomePath()+"/"+name;
	if(Util::fileExists(fullName)){
		if(unlink(fullName.c_str())){
			syslog(LOG_ERR,"Can't delete file %s: %m",name.c_str());
			throw "Jail error: can't delete file";
		}
	}
}

void Jail::catchSIGTERM(int n){
		//Do nothing
}

/**
 * chdir and chroot to jail
 */
void Jail::goJail(){
	if(chdir(jailPath.c_str()) != 0) throw "Can't chdir to jail";
	if(chroot(jailPath.c_str()) != 0) throw "Can't chroot to jail";
	syslog(LOG_INFO,"chrooted \"%s\"",jailPath.c_str());
}

/**
 * Root mutate to be prisoner
 */
void Jail::changeUser(){
	if(setresgid(prisoner,prisoner,prisoner)!= 0) {
		syslog(LOG_ERR,"Can't change to prisoner group %d: %m",prisoner);
		throw "Can't change to prisoner group";
	}
	if(setresuid(prisoner,prisoner,prisoner)!= 0){
		syslog(LOG_ERR,"Can't change to prisoner user %d: %m",prisoner);
		throw "Can't change to prisoner user";
	}
	//Recheck not needed
	if(getuid()==0 || geteuid()==0) throw "Can't change to prisoner user 2";
	syslog(LOG_INFO,"change user to %d",prisoner);
}

/**
 * Execute program at prisoner home directory
 */
void Jail::transferExecution(string fileName){
	 string dir=prisonerHomePath();
	 string fullname=dir+"/"+fileName;
     if(chdir(dir.c_str())){
     	syslog(LOG_ERR,"Can't chdir to home dir. \"%s\" %m",dir.c_str());
     	throw "Can't chdir to exec dir";
     }
     char *argv[6];
     char *command= new char[fullname.size()+1];
     strcpy(command,fullname.c_str());
     setpgrp();
     int i=0;
     argv[i++] = command;
     argv[i++] = NULL;
     syslog(LOG_ERR,"Running \"%s\"",command);
     execve(command,argv,environment);
     syslog(LOG_ERR,"execve \"%s\" fail: %m",command);
     throw "Can't execve";
}

/**
 * set user limits
 */
void Jail::setLimits(){
	//
	struct rlimit limit;
	limit.rlim_cur=executionLimits.maxmemory;
	limit.rlim_max=executionLimits.maxmemory;
	setrlimit(RLIMIT_AS,&limit);
	setrlimit(RLIMIT_DATA,&limit);
	setrlimit(RLIMIT_STACK,&limit);
	limit.rlim_cur=0;
	limit.rlim_max=0;
	setrlimit(RLIMIT_CORE,&limit);
	limit.rlim_cur=executionLimits.maxtime;
	limit.rlim_max=executionLimits.maxtime;
	setrlimit(RLIMIT_CPU,&limit);
	limit.rlim_cur=executionLimits.maxfilesize;
	limit.rlim_max=executionLimits.maxfilesize;
	setrlimit(RLIMIT_FSIZE,&limit);
	limit.rlim_cur=executionLimits.maxprocesses;
	limit.rlim_max=executionLimits.maxprocesses;
	setrlimit(RLIMIT_NPROC,&limit);
	//RLIMIT_MEMLOCK
	//RLIMIT_MSGQUEUE
	//RLIMIT_NICERLIMIT_NOFILE
	//RLIMIT_RTPRIO
	//RLIMIT_SIGPENDING

}

/**
 * run program controlling timeout and redirection
 */
string Jail::run(string name, int host, int port, string password){
	int fdmaster=-1;
	signal(SIGTERM,Jail::catchSIGTERM);
	signal(SIGKILL,Jail::catchSIGTERM);
	newpid=forkpty(&fdmaster,NULL,NULL,NULL);
	if (newpid == -1) //fork error
		return "Jail: fork error";
	if(newpid == 0){ //new process
		try{
			goJail();
			setLimits();
			changeUser();
			transferExecution(name);
		}catch(const char *s){
			syslog(LOG_INFO,"Error running: %s",s);
			printf("\nJail error: %s\n",s);
		}
		catch(...){
			syslog(LOG_INFO,"Error running");
			printf("\nJail error: at execution stage\n");
		}
		exit(EXIT_SUCCESS);
  	}
  	syslog(LOG_INFO, "child pid %d",newpid);
  	Redirector redirector;
  	if(host){
  	    redirector.start(fdmaster,host,port,password);
  	}else{
  		redirector.start(fdmaster);
  	}
     const long int waittime= 10000;
     volatile time_t startTime=time(NULL);
     //struct rusage resources;
     int status;
     while(redirector.isActive()){
    	 redirector.advance();
 	    pid_t wret=waitpid(newpid, &status, WNOHANG);
        if(wret == newpid){
			if(WIFSIGNALED(status)){
				int signal = WTERMSIG(status);
				char buf[1000];
				sprintf(buf,"\nJail: program terminated due to \"%s\" (%d)\n",
						strsignal(signal),signal);
				redirector.addMessage(buf);
			}else if(WIFEXITED(status)){
				int exitcode = WEXITSTATUS(status);
				if(exitcode != EXIT_SUCCESS){
					char buf[100];
					sprintf(buf,"\nJail: program terminated normally with exit code %d.\n",exitcode);
					redirector.addMessage(buf);
				}
			}else{
				redirector.addMessage("Jail: program terminated but unknown reason.");
			}
        	newpid=-1;
        	break;
        } else if(wret > 0){//waitpid error wret != newpid
        	redirector.addMessage("\nJail waitpid error: ret>0.\n");
        	newpid=-1;
        	break;
        } else if(wret == -1) { //waitpid error
        	newpid=-1;
        	redirector.addMessage("\nJail waitpid error: ret==-1.\n");
  			syslog(LOG_INFO,"Jail waitpid error: %m");
 			break;
        }
        if (wret == 0){ //Process running
           time_t now=time(NULL);
           if(now-startTime > executionLimits.maxtime){
        	   redirector.addMessage("\nJail: execution time limit reached.\n");
           		syslog(LOG_INFO,"Execution time limit (%d) reached",executionLimits.maxtime);
				break;
           }
        }
     	usleep(waittime);
     }
     if(newpid>0){
     	stopPrisonerProcess();
     }
	//wait until 5sg for redirector to read and send program output
	for(int i=0;redirector.isActive() && i<50; i++){
		redirector.advance();
		usleep(100000); // 1/10 seg
	}
	string output=redirector.getOutput();
	syslog(LOG_INFO,"Complete program output: %s", output.c_str());
    return output;
}
