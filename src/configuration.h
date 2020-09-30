/**
 * package:		Part of vpl-jail-system
 * copyright:	Copyright (C) 2009 Juan Carlos Rodríguez-del-Pino. All rights reserved.
 * license:		GNU/GPL, see LICENSE.txt or http://www.gnu.org/licenses/gpl-3.0.html
 **/
#ifndef CONFIGURATION_INC_H
#define CONFIGURATION_INC_H

#include <stdlib.h>
#include <regex.h>
#include <string>
#include <vector>
#include "util.h"
using namespace std;
class Configuration{
private:
	static Configuration* singlenton;
	ExecutionLimits jailLimits;
	string jailPath;   //Path to jail file system
	string cleanPATH; //Path environment variable cleaned
	string configPath; //Path to configuration file
	string controlPath; //Path to the control directory
	uid_t  minPrisoner, maxPrisoner; //uid prisoner selection limits
	vector<string> taskOnlyFrom; //Moodle servers that can submit task
	string interface; //Interface to serve default all
	string URLPath; //URL path to accept tasks
	int port, sport; //Server ports
	int logLevel; //Log level 0 to 8, 0 = no log, 8 += do not remove home dir.
	int fail2ban; //Fail to ban configuration. Relation fail / success to ban IP
	string SSLCipherList;
	string SSLCertFile;
	string SSLKeyFile;
	regex_t reg;
	void checkConfigFile(string fileName, string men);
	Configuration();
public:
	static Configuration* getConfiguration(){
		if(singlenton == NULL) singlenton= new Configuration();
		return singlenton;
	}
	void readConfigFile();
	static string generateCleanPATH(string jailPath, string dirtyPATH);
	const ExecutionLimits & getLimits() const { return jailLimits;}
	const string & getJailPath() const { return jailPath;}
	const string &  getCleanPATH() const { return cleanPATH;}
	const string &  getControlPath() const { return controlPath;}
	size_t getMinPrisoner() const { return minPrisoner;}
	size_t getMaxPrisoner() const { return maxPrisoner;}
	const vector<string> & getTaskOnlyFrom() const { return taskOnlyFrom;}
	const string &  getInterface() const { return interface;}
	const string &  getURLPath() const { return URLPath;}
	int getPort() const { return port;}
	int getSecurePort() const { return sport;}
	int getLogLevel() const { return logLevel;}
	int getFail2Ban() const { return fail2ban;}
	const string &  getSSLCipherList() const { return SSLCipherList;}
	const string &  getSSLCertFile() const { return SSLCertFile;}
	const string &  getSSLKeyFile() const { return SSLKeyFile;}
};
#endif



