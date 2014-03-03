/**
 * version:		$Id: configuration.h,v 1.7 2014-02-21 17:51:29 juanca Exp $
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
	string configPath; //Path to configuration file
	string controlPath; //Path to the control directory
	uid_t  minPrisoner, maxPrisoner; //uid prisoner selection limits
	vector<string> taskOnlyFrom; //Moodle servers that can submit task
	string interface; //Interface to serve default all
	string URLPath; //URL path to accept tasks
	int port, sport; //Server ports
	regex_t reg;
	void checkConfigFile(string fileName, string men);
	void readConfigFile();
	Configuration();
public:
	static Configuration* getConfiguration(){
		if(singlenton == NULL) singlenton= new Configuration();
		return singlenton;
	}
	ExecutionLimits getLimits(){ return jailLimits;}
	string getJailPath(){ return jailPath;}
	string getControlPath(){ return controlPath;}
	size_t getMinPrisioner(){ return minPrisoner;}
	size_t getMaxPrisioner(){ return maxPrisoner;}
	const vector<string> & getTaskOnlyFrom(){ return taskOnlyFrom;}
	string getInterface(){ return interface;}
	string getURLPath(){ return URLPath;}
	int getPort(){ return port;}
	int getSecurePort(){ return sport;}
};
#endif



