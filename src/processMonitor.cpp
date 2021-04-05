/**
 * @package:   Part of vpl-jail-system
 * @copyright: Copyright (C) 2019 Juan Carlos Rodríguez-del-Pino
 * @license:   GNU/GPL, see LICENSE.txt or http://www.gnu.org/licenses/gpl-3.0.html
 **/
#include <list>
using namespace std;
#include <limits.h>
#include <syslog.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <wait.h>
#include "lock.h"
#include "processMonitor.h"
#include "vpl-jail-server.h"
/**
 * Return the number of prisoners in jail
 */
int processMonitor::requestsInProgress(){
	const string homeDir=Configuration::getConfiguration()->getJailPath()+"/home";
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
	return nprisoner;
}

/**
 * Root mutate to be prisoner
 */
void processMonitor::becomePrisoner(int prisoner){
	if(setresgid(prisoner,prisoner,prisoner)!= 0) {
		throw HttpException(internalServerErrorCode
				,"I can't change to prisoner group",Util::itos(prisoner));
	}
	if(setresuid(prisoner,prisoner,prisoner)!= 0){
		throw HttpException(internalServerErrorCode
				,"I can't change to prisoner user",Util::itos(prisoner));
	}
	//Recheck not needed
	if(getuid()==0 || geteuid()==0)
		throw HttpException(internalServerErrorCode
				,"I can't change to prisoner user 2",Util::itos(prisoner));
	Daemon::closeSockets();
	syslog(LOG_INFO,"change user to %d",prisoner);
}

/**
 * Root mutate to be prisoner
 */
void processMonitor::becomePrisoner(){
	becomePrisoner(prisoner);
}

void processMonitor::stopPrisonerProcess(bool soft){
	stopPrisonerProcess(prisoner, soft);
}

void processMonitor::stopPrisonerProcess(int prisoner, bool soft){
	syslog(LOG_INFO,"Sttoping prisoner process");
	pid_t pid=fork();
	if(pid==0){ //new process
		becomePrisoner(prisoner);
		if(soft){
			//To not stop at first signal
			signal(SIGTERM,catchSIGTERM);
			kill(-1,SIGTERM);
			usleep(100000);
		}
		kill(-1,SIGKILL);
		_exit(EXIT_SUCCESS);
	}
	else{
		for(int i=0;i <50 ; i++){
			usleep(100000);
			if(pid==waitpid(pid,NULL,WNOHANG)) //Wait for pid end
				break;
		}
	}
}

/**
 * return prisoner home page
 * if user root absolute path if prisoner relative to jail path
 */
string processMonitor::prisonerHomePath(){
	char buf[100];
	sprintf(buf,"/home/p%d",prisoner);
	uid_t uid=getuid();
	if(uid!=0){
		syslog(LOG_ERR,"Security problem? prisonerHomePath %d",uid);
		return buf;
	}
	return configuration->getJailPath()+buf;
}

/**
 * Select a prisoner id from minPrisoner to maxPrisoner
 * Create prisoner home dir
 */
void processMonitor::selectPrisoner(){
	int const range = configuration->getMaxPrisoner() - configuration->getMinPrisoner() + 1;
	int const ntry = range;
	int const umask = 0700;
	Lock lock(configuration->getControlPath());
	for(int i = 0; i < ntry; i++){
		setPrisonerID(configuration->getMinPrisoner() + (Util::random() + i) % range);
		string controlPath = getProcessControlPath();
		if( mkdir(controlPath.c_str(), umask) == 0 ) {
			homePath = prisonerHomePath();
			if( mkdir(homePath.c_str(), umask) == 0 ){
				if( chown(homePath.c_str(), prisoner, prisoner) ) {
					throw HttpException(internalServerErrorCode
							, "I can't change prisoner home dir \"" + homePath +
							  "\" to proper owner: " + strerror(errno));
				}
				return;
			} else if ( errno == EEXIST) {
				syslog(LOG_ERR, "Exists home dir without control dir, removing ... %s", homePath.c_str());
				Util::removeDir(homePath, getPrisonerID(), true);
				Util::removeDir(configuration->getJailPath() + "/tmp", getPrisonerID(), false);
				rmdir(controlPath.c_str());
				continue;
			}
			throw HttpException(internalServerErrorCode
							, "I can't create prisoner home dir \"" + homePath +
							  "\" : " + strerror(errno));
		} else if ( errno == EEXIST) {
			continue;
		}
		throw HttpException(internalServerErrorCode
						, "I can't create prisoner control dir \"" + Util::itos(prisoner) +
							  "\" : "+ strerror(errno));
	}
	throw HttpException(internalServerErrorCode
			,"I can't select prisoner home dir: free UID not found");
}

void processMonitor::writeInfo(ConfigData data){
	string configFile = getProcessControlPath() + "/config";
	//Read current config and don't lost data not set now
	if(Util::fileExists(configFile)){
		data = ConfigurationFile::readConfiguration(configFile, data);
	}
	data["MAXTIME"] = Util::itos(executionLimits.maxtime);
	data["MAXFILESIZE"] = Util::itos(executionLimits.maxfilesize);
	data["MAXMEMORY"] = Util::itos(executionLimits.maxmemory);
	data["MAXPROCESSES"] = Util::itos(executionLimits.maxprocesses);

	data["ADMINTICKET"] = adminticket;
	data["EXECUTIONTICKET"] = executionticket;
	data["MONITORTICKET"] = monitorticket;

	data["STARTTIME"] = Util::itos(startTime);
	data["INTERACTIVE"] = interactive?"1":"0";
	data["LANG"] = lang;
	data["COMPILER_PID"] = Util::itos(compiler_pid);
	data["RUNNER_PID"] = Util::itos(runner_pid);
	data["MONITOR_PID"] = Util::itos(monitor_pid);
	ConfigurationFile::writeConfiguration(configFile, data);
}

ConfigData processMonitor::readInfo(){
	ConfigData data;
	data=ConfigurationFile::readConfiguration(getProcessControlPath("config"), data);
	executionLimits.maxtime = atoi(data["MAXTIME"].c_str());
	executionLimits.maxfilesize = atoi(data["MAXFILESIZE"].c_str());
	executionLimits.maxmemory = atoi(data["MAXMEMORY"].c_str());
	executionLimits.maxprocesses = atoi(data["MAXPROCESSES"].c_str());

	adminticket = data["ADMINTICKET"];
	executionticket = data["EXECUTIONTICKET"];
	monitorticket = data["MONITORTICKET"];

	startTime = atoi(data["STARTTIME"].c_str());
	interactive = atoi(data["INTERACTIVE"].c_str());
	lang = data["LANG"];
	compiler_pid = atoi(data["COMPILER_PID"].c_str());
	runner_pid = atoi(data["RUNNER_PID"].c_str());
	monitor_pid = atoi(data["MONITOR_PID"].c_str());
	return data;
}

processMonitor::processMonitor(string & adminticket, string & monitorticket, string & executionticket){
	const int salt=127;
	configuration = Configuration::getConfiguration();
	security = admin;
	string cp=configuration->getControlPath();
	adminticket = Util::itos(Util::random()/salt);
	monitorticket = Util::itos(Util::random()/salt);
	executionticket = Util::itos(Util::random()/salt);
	selectPrisoner();
	executionticket += Util::itos(Util::random()/salt);
	monitorticket += Util::itos(Util::random()/salt);
	adminticket += Util::itos(Util::random()/salt);
	{
		Lock lock(cp);
		while(Util::fileExists(cp+"/"+monitorticket)
		||Util::fileExists(cp+"/"+executionticket)
		||Util::fileExists(cp+"/"+adminticket)){
			adminticket = Util::itos(Util::random()/salt);
			monitorticket = Util::itos(Util::random()/salt);
			executionticket = Util::itos(Util::random()/salt);
			executionticket += Util::itos(Util::random()/salt);
			monitorticket += Util::itos(Util::random()/salt);
			adminticket += Util::itos(Util::random()/salt);
		}
		//Write tickets
		ConfigData data;
		data["USER_ID"] = Util::itos(prisoner);
		data["SECURITY"] = Util::itos(admin);
		ConfigurationFile::writeConfiguration(cp + "/" + adminticket, data);
		data["SECURITY"] = Util::itos(monitor);
		ConfigurationFile::writeConfiguration(cp + "/" + monitorticket, data);
		data["SECURITY"] = Util::itos(execute);
		ConfigurationFile::writeConfiguration(cp + "/" + executionticket, data);
		executionLimits.maxtime = 0;
		executionLimits.maxfilesize = 0;
		executionLimits.maxmemory = 0;
		executionLimits.maxprocesses = 0;
		this->adminticket = adminticket;
		this->monitorticket = monitorticket;
		this->executionticket = executionticket;
		startTime = time(NULL);
		interactive=false;
		compiler_pid=0;
		monitor_pid=0;
		runner_pid=0;
		writeInfo();
	}
}

processMonitor::processMonitor(string ticket){
	configuration = Configuration::getConfiguration();
	bool error=true;
	Util::trimAndRemoveQuotes(ticket);
	regex_t reg;
	regcomp(&reg,"^[0-9]+$", REG_EXTENDED);
	regmatch_t match[1];
	int nomatch=regexec(&reg, ticket.c_str(),1, match, 0);
	regfree(&reg);
	if(nomatch == 0){
		Lock lock(configuration->getControlPath());
		string fileName=configuration->getControlPath() + "/" + ticket;
		if(Util::fileExists(fileName)){
			ConfigData data;
			data["USER_ID"] = "0";
			data["SECURITY"] = "3"; //none
			data = ConfigurationFile::readConfiguration(fileName,data);
			setPrisonerID (atoi(data["USER_ID"].c_str()));
			security = (securityLevel) atoi(data["SECURITY"].c_str());
			if(security == monitor || security == execute)
				Util::deleteFile(fileName);//Anulate tikect
			string configFile = getProcessControlPath("config");
			if(Util::fileExists(configFile)){
				readInfo();
				error=false;
			}
			if(security == monitor)
				monitorize();
		}
	}
	if(error){
		throw "Ticket invalid";
	}
}

bool processMonitor::FileExists(string name){
	return Util::fileExists(getHomePath()+"/"+name);
}

bool processMonitor::controlFileExists(string name){
	return Util::fileExists(getProcessControlPath(name));
}

string processMonitor::readFile(string name){
	return Util::readFile(getHomePath()+"/"+name);
}

void processMonitor::writeFile(string name, const string &data){
	string homePath=getHomePath();
	string fullName=homePath+"/"+name;
	bool isScript=name.size()>4 && name.rfind(".sh",name.size()-3)!=string::npos;
	if(isScript){ //Endline converted to linux
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
		Util::writeFile(fullName, newdata, getPrisonerID(), homePath.size() + 1);
	}else{
		Util::writeFile(fullName, data, getPrisonerID(), homePath.size() + 1);
	}
}

/**
 * Delete a file from prisoner home directory
 */
void processMonitor::deleteFile(string name){
	Util::deleteFile(getHomePath() + "/" + name);
}

bool processMonitor::installScript(string to, string from){
	if(Util::fileExists("/usr/sbin/vpl/" + from)){
		string scriptCode=Util::readFile("/usr/sbin/vpl/" + from);
		syslog(LOG_DEBUG, "Installing %s in %s", to.c_str(), from.c_str());
		writeFile(to, scriptCode);
		return true;
	}
	return false;
}

bool processMonitor::isRunnig(){
	return getState() != stopped;
}

processState processMonitor::getState(){
	if(!Util::dirExists(getProcessControlPath())) return stopped;
	Lock lock(getProcessControlPath());
	string fileName = getProcessControlPath("config");
	if(!Util::fileExists(fileName))	return stopped;
	readInfo();
	if(compiler_pid==0) return starting;
	time_t currentTime = time(NULL);
	if (startTime > currentTime || startTime == 0 ) {
		syslog(LOG_ERR,"Internal error startTime bad %ld", startTime);
		return starting;
	}
	time_t elapsedTime = currentTime - startTime;
	time_t tlimit = startTime;
	tlimit += 2 * executionLimits.maxtime;
	tlimit += JAIL_HARVEST_TIMEOUT;
	if (tlimit < currentTime) {
		syslog(LOG_INFO,"Execution last timeout reached %ld. ", tlimit);
		cleanTask();
		return stopped;
	}
	bool aliveCompiler=Util::processExists(compiler_pid);
	if(aliveCompiler && runner_pid==0) return compiling;
	if(monitor_pid == 0 && elapsedTime > JAIL_MONITORSTART_TIMEOUT) {
		syslog(LOG_INFO,"Execution without monitor timeout reached %d. ", JAIL_MONITORSTART_TIMEOUT);
		cleanTask();
		return stopped;
	}
	if(runner_pid==0){
		if(interactive) return beforeRunning;
		if(controlFileExists("compilation")) return retrieve;
		syslog(LOG_INFO,"Execution stopped, not interactive, not runner, no compilation file");
		return stopped;
	}
	bool aliveRunner=Util::processExists(runner_pid);
	if(aliveRunner) return running;
	if(interactive) return stopped;
	if(controlFileExists("execution")) return retrieve;
	return stopped;
}

void processMonitor::setRunner(){
	if (security == monitor) return;
	Lock lock(getProcessControlPath());
	readInfo();
	runner_pid = getpid();
	writeInfo();
}
void processMonitor::setCompiler(){
	if (security == monitor) return;
	Lock lock(getProcessControlPath());
	readInfo();
	startTime = time(NULL);
	compiler_pid = getpid();
	writeInfo();
}

bool processMonitor::isMonitored(){
	if(!Util::dirExists(getProcessControlPath())) return false;
	Lock lock(getProcessControlPath());
	if ( monitor == 0 ) readInfo();
	if ( monitor == 0 ) return false;
	return Util::processExists(monitor);
}

void processMonitor::monitorize(){
	if ( security != monitor ) return;
	Lock lock(getProcessControlPath());
	readInfo();
	if(monitor_pid != 0)
		throw string("Process already monitorized");
	monitor_pid = getpid();
	writeInfo();
}

void processMonitor::setExtraInfo(ExecutionLimits el, bool ri, string lang){
	Lock lock(getProcessControlPath());
	readInfo();
	executionLimits = el;
	interactive = ri;
	this->lang = lang;
	writeInfo();
}

void processMonitor::limitResultSize(string &r){
	if (r.size()
			>= static_cast<unsigned int>(configuration->getRequestMaxSize())) {
		string men="\nThis output has been cut to "+Util::itos(configuration->getRequestMaxSize()/1024)+"Kb"
				+", its original size was "+Util::itos(r.size()/1024)+"Kb\n";
		r = men+r.substr(0,configuration->getRequestMaxSize()/2)+men+r.substr(r.size()-configuration->getRequestMaxSize()/2);
	}
}
void processMonitor::getResult(string &compilation, string &execution, bool &executed){
	if(security != admin)
		throw HttpException(internalServerErrorCode,"Security: required admin ticket for request");
	if(isInteractive())
		throw HttpException(internalServerErrorCode,"Security: process in bad state");
	{
		string fileName;
		Lock lock(getProcessControlPath());
		compilation="";
		execution="";
		executed=false;
		fileName=getProcessControlPath("compilation");
		if(Util::fileExists(fileName)){
			compilation=Util::readFile(fileName);
			limitResultSize(compilation);
			Util::deleteFile(fileName);
		}
		fileName=getProcessControlPath("execution");
		if((executed=Util::fileExists(fileName))){
			execution=Util::readFile(fileName);
			limitResultSize(execution);
			Util::deleteFile(fileName);
		}
	}
}

string processMonitor::getCompilation(){
	if(security != monitor)
		throw HttpException(internalServerErrorCode,"Security: required monitor ticket for getCompilation");
	{
		Lock lock(getProcessControlPath());
		string fileName = getProcessControlPath("compilation");
		if(Util::fileExists(fileName))
			return Util::readFile(fileName);
		return "";
	}
}

void processMonitor::setCompilationOutput(const string &compilation){
	string fileName;
	Lock lock(getProcessControlPath());
	fileName = getProcessControlPath("compilation");
	if(Util::fileExists(fileName))
		throw "Compilation already save";
	Util::writeFile(fileName,compilation);
}
void processMonitor::setExecutionOutput(const string &execution, bool executed){
	string fileName;
	Lock lock(getProcessControlPath());
	fileName = getProcessControlPath("compilation");
	if(!Util::fileExists(fileName))
		throw "Compilation not saved";
	if(executed){
		fileName = getProcessControlPath("execution");
		if(Util::fileExists(fileName))
			throw "Execution already saved";
		Util::writeFile(fileName, execution);
	}
}


/**
 * remove a directory and its content
 * if force=true remove always
 * else remove files owned by prisoner
 * and complete directories owned by prisoner (all files and directories owns by prisoner or not)
 */
int processMonitor::removeDir(string dir, bool force){
	return Util::removeDir(dir, prisoner, force);
}

/**
 * remove prisoner home directory and /tmp prisoner files
 */
void processMonitor::removePrisonerHome(){
	if ( configuration->getLogLevel() != 8 ) {
		syslog(LOG_INFO, "Remove prisoner home (%d)", prisoner);
		Util::removeDir(configuration->getJailPath() + "/tmp", getPrisonerID(), false); //Only prisoner files and dirs
		Util::removeDir(prisonerHomePath(), getPrisonerID(), true); //All files and dir
	} else {
		syslog(LOG_INFO,"Loglevel = 8 => do not remove prisoner home (%d)",prisoner);
	}
}

/**
 * Remove prisoner files and stop prisoner process
 */

/**
 * Remove task home dir and monitor control files
 */
void processMonitor::cleanTask(){
	syslog(LOG_INFO,"Cleaning task");
	stopPrisonerProcess(false);
	removePrisonerHome();
	string controlPath = configuration->getControlPath();
	Lock lock(controlPath);

	Util::removeDir(getProcessControlPath(), getPrisonerID(), true);
	Util::deleteFile(controlPath + "/" + adminticket);
	Util::deleteFile(controlPath + "/" + monitorticket);
	Util::deleteFile(controlPath + "/" + executionticket);

	usleep(100000);
	stopPrisonerProcess(true);
	removePrisonerHome();
}

bool processMonitor::isOutOfMemory(){
	if(executionLimits.maxmemory==0) return false;
	return (uint64_t) executionLimits.maxmemory < getMemoryUsed();
}

string processMonitor::getMemoryLimit(){
	if(executionLimits.maxmemory==0) return "unlimited";
	return Util::itos(executionLimits.maxmemory/(1024*1024))+"MiB";
}

uint64_t processMonitor::getMemoryUsed(){
	string dir="/proc";
	dirent *ent;
	DIR *dirfd=opendir(dir.c_str());
	if(dirfd==NULL){
		syslog(LOG_ERR,"Can't open \"/proc\" dir: %m");
		return INT_MAX; //
	}
	static bool init=false;
	static regex_t reg_uid;
	static regex_t reg_mem;
	if(!init){
		regcomp(&reg_uid, ".*^Uid:[ \t]+([0-9]+)", REG_EXTENDED|REG_ICASE|REG_NEWLINE);
		regcomp(&reg_mem, ".*^VmHWM:[ \t]+([0-9]+)[ \t]+(.*)", REG_EXTENDED|REG_ICASE|REG_NEWLINE);
		init=true;
	}
	const int matchSize = 3;
	regmatch_t match[matchSize];
	uint64_t s=0;
	while((ent=readdir(dirfd))!=NULL){
		const string name(ent->d_name);
		const pid_t PID = Util::atoi(name);
		if(Util::itos(PID) != name ) continue;
		const string statusFile = dir + "/" + name + "/status";
		string status = Util::readFile(statusFile, false);
		int nomatch=regexec(&reg_uid, status.c_str(), matchSize, match, 0);
		if(nomatch==0){
			string UIDF=status.substr(match[1].rm_so, match[1].rm_eo - match[1].rm_so);
			if(Util::atoi(UIDF) == (int) prisoner){
				nomatch = regexec(&reg_mem, status.c_str(), matchSize, match, 0);
				if(nomatch == 0){
					string MEM = status.substr(match[1].rm_so, match[1].rm_eo - match[1].rm_so);
					string MUL = status.substr(match[2].rm_so, match[2].rm_eo - match[2].rm_so);
					const int onek = 1024;
					int mul = onek; //Default kB 1024
					if(MUL == "mB") mul *= onek;
					s += Util::atol(MEM) * mul;
				}
			}
		}
	}
	closedir(dirfd);
	return s;
}

void processMonitor::freeWatchDog(){
	//TODO check process running based on /proc
	//looking for inconsistencies
}

vector<string> processMonitor::getPrisonersFromDir(string dir) {
	size_t minUid = Configuration::getConfiguration()->getMinPrisoner();
	size_t maxUid = Configuration::getConfiguration()->getMaxPrisoner();
	vector<string> prisoners;
	dirent *ent;
	DIR *dirfd=opendir(dir.c_str());
	if(dirfd==NULL){
		syslog(LOG_ERR,"Can't open dir \"%s\": %m",dir.c_str());
		return prisoners;
	}
	while((ent=readdir(dirfd))!=NULL){
		if (ent->d_type == DT_DIR) {
			const string name(ent->d_name);
			if(name.at(0) == 'p') {
				size_t uid = (size_t) Util::atoi(name.substr(1));
				if (uid >= minUid && uid <= maxUid) {
					prisoners.push_back(name);
				}
			}
		}
	}
	closedir(dirfd);
	return prisoners;
}

void processMonitor::cleanPrisonerFiles(string pdir) {
	syslog(LOG_INFO,"Cleaning prisoner files");
	const string controlDir = Configuration::getConfiguration()->getControlPath();
	const string jailPath = Configuration::getConfiguration()->getControlPath();
	const string phome = jailPath + "/home" + "/" + pdir;
	const string tmpDir = jailPath + "/tmp";
	const string configDir = controlDir + "/" + pdir;
	const string configFile = configDir + "/" + "config";
	ConfigData data;

	try {
		data = ConfigurationFile::readConfiguration(configFile, data);
	} catch(...) {}
	if ( data["ADMINTICKET"] > "") {
		Util::deleteFile( controlDir + "/" + data["ADMINTICKET"] );
	}
	if ( data["EXECUTIONTICKET"] > "") {
		Util::deleteFile( controlDir + "/" + data["EXECUTIONTICKET"] );
	}
	if ( data["MONITORTICKET"] > "") {
		Util::deleteFile( controlDir + "/" + data["MONITORTICKET"] );
	}

	int uid = Util::atoi(pdir.substr(1));
	try {
		Util::removeDir(phome, uid, true);
	} catch(...) {}
	try {
		Util::removeDir(tmpDir, uid, false);
	}catch(...) {}
	try {
		Util::removeDir(configDir, uid, true);
	}catch(...) {}
}

void processMonitor::cleanZombieTasks() {
	syslog(LOG_INFO,"Cleaning zombie tasks");
	const string controlDir=Configuration::getConfiguration()->getControlPath();
	const string homeDir=Configuration::getConfiguration()->getJailPath() + "/home";
	vector<string> homes = getPrisonersFromDir(homeDir);
	vector<string> tasks = getPrisonersFromDir(controlDir);
	for (size_t i = 0; i < tasks.size(); i++) {
		ConfigData data;
		syslog(LOG_INFO,"Cleaning zombie tasks: checking(1) %s", tasks[i].c_str());
		string configDir = controlDir + "/" + tasks[i];
		string configFile = configDir + "/" + "config";
		struct stat statbuf;
		try {
			stat(configDir.c_str(), &statbuf);
			if ( statbuf.st_mtime + JAIL_MONITORSTART_TIMEOUT < time(NULL) ) {
				data = ConfigurationFile::readConfiguration(configFile, data);
				processMonitor pm(data["ADMINTICKET"]);
				pm.getState();
			}
		}catch(...) {
			time_t tlimit = statbuf.st_mtime;
			tlimit += Configuration::getConfiguration()->getLimits().maxtime;
			tlimit += JAIL_HARVEST_TIMEOUT;
			if ( tlimit < time(NULL) ) {
				cleanPrisonerFiles(tasks[i]);
			}
		}
	}
	Lock lock(controlDir);
	for (size_t i = 0; i < homes.size(); i++) {
		syslog(LOG_INFO, "Cleaning zombie tasks: checking(2) %s", homes[i].c_str());
		string configFile = controlDir + "/" + homes[i] + "/" + "config";
		string phome = homeDir + "/" + homes[i];
		if ( Util::dirExists(phome) && ! Util::fileExists(configFile)) {
			struct stat statbuf;
			stat(phome.c_str(), &statbuf);
			if ( statbuf.st_mtime + (JAIL_MONITORSTART_TIMEOUT * 2) < time(NULL) ) {
				cleanPrisonerFiles( homes[i] );
			}
		}
	}
}
