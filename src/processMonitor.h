/**
 * @package:   Part of vpl-jail-system
 * @copyright: Copyright (C) 2014 Juan Carlos Rodríguez-del-Pino
 * @license:   GNU/GPL, see LICENSE.txt or http://www.gnu.org/licenses/gpl-3.0.html
 **/
#ifndef PROCESSMONITOR_INC_H
#define PROCESSMONITOR_INC_H

#include <stdint.h>

#include <string>
#include <vector>
using namespace std;
#include "configurationFile.h"
#include "configuration.h"

enum securityLevel{
	admin,monitor,execute,none
};
enum processState{
	prestarting,starting,compiling,beforeRunning,running,retrieve,stopped
};

class processMonitor{
	uid_t  prisoner;   //User prisoner selected
	pid_t  compiler_pid; //Compiler pid
	pid_t  runner_pid;    //runner pid
	pid_t  monitor_pid;
	string adminticket;
	string monitorticket;
	string executionticket;
	bool   interactive; //is interactive request
	string lang; //Code of language to use
	time_t startTime; //Start time of the request
	ExecutionLimits executionLimits;
	processState state;
	securityLevel security;
	Configuration *configuration;
	string processControlPath; //Folder with process info
	string homePath;
	void writeInfo(ConfigData data=ConfigData());
	ConfigData readInfo();
	void writeState();
	void readState();
	string prisonerHomePath();
	void selectPrisoner();
	void removePrisonerHome();
	static void catchSIGTERM(int n){}
	void limitResultSize(string &);
	void setPrisonerID(uid_t  prisoner) {
		this->prisoner = prisoner;
		setProcessControlPath();
	}
	void setProcessControlPath(){
		processControlPath = configuration->getControlPath() + "/p" + Util::itos(getPrisonerID());
	}
	string getProcessControlPath(){
		return processControlPath;
	}
	string getProcessControlPath(string name){
		return processControlPath+"/"+name;
	}
	static void cleanPrisonerFiles(string);
public:
	processMonitor(string & adminticket, string & monitorticket, string & executionticket);
	processMonitor(string ticket);
	bool FileExists(string);
	bool controlFileExists(string);
	string readFile(string);
	void writeFile(string, const string &);
	void deleteFile(string);
	int removeDir(string dir, bool force);
	bool installScript(string to, string from);
	static int requestsInProgress();
	void setExtraInfo(ExecutionLimits, bool interactive, string lang);
	string getLang(){ return lang;}
	string getVNCPassword(){ return executionticket.substr(0,8);}
	time_t getStartTime(){return startTime;}
	time_t getMaxTime(){return executionLimits.maxtime;}
	bool isInteractive(){return interactive;}
	ExecutionLimits getLimits(){return executionLimits;}
	uid_t getPrisonerID(){return prisoner;}
	securityLevel getSecurityLevel(){return security;}
	string getHomePath(){return configuration->getJailPath()+"/home/p"+Util::itos(prisoner);}
	string getRelativeHomePath(){return "/home/p"+Util::itos(prisoner);}
	void becomePrisoner();
	void becomePrisoner(int);
	bool isRunnig();
	void stopPrisonerProcess(bool);
	void stopPrisonerProcess(int, bool);
	void cleanTask();
	processState getState();
	void setCompiler();
	void setRunner();
	bool isMonitored();
	void monitorize();
	bool isOutOfMemory();
	uint64_t getMemoryUsed();
	string getMemoryLimit();
	string getCompilation();
	void getResult(string &compilation, string &execution, bool &executed);
	void setCompilationOutput(const string &compilation);
	void setExecutionOutput(const string &execution, bool executed);
	void freeWatchDog();
	static vector<string> getPrisonersFromDir(string dir);
	static void cleanZombieTasks();
};
#endif
