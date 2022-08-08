/**
 * package:		Part of vpl-jail-system
 * copyright:	Copyright (C) 2014 Juan Carlos Rodríguez-del-Pino. All rights reserved.
 * license:		GNU/GPL, see LICENSE.txt or http://www.gnu.org/licenses/gpl-3.0.html
 **/
#include <sys/syslog.h>
#include <climits>
#include <sstream>
#include "configuration.h"
#include "configurationFile.h"

#include "jail_limits.h"
#include "util.h"
Configuration *Configuration::singlenton=0;
string Configuration::generateCleanPATH(string path, string dirtyPATH){
	string dir;
	size_t pos=0;
	size_t found;
	string clean;
	while(true){
		if((found=dirtyPATH.find(':', pos)) != string::npos){
			dir = dirtyPATH.substr(pos,found-pos);
		}else{
			dir = dirtyPATH.substr(pos);
		}
		if(Util::dirExistsFollowingSymLink(path+dir)){
			if(clean.size()>0){
				clean += ':';
			}
			clean += dir;
		} else {
			syslog(LOG_INFO, "Path removed: %s", dir.c_str());
		}
		if(found == string::npos) return clean;
		pos=found+1;
	}
	return "";
}

void Configuration::checkConfigFile(string fileName, string men){
	const int ga_mask = 077;
	struct stat info;
	if(lstat(fileName.c_str(), &info))
		throw men + " not found " + fileName;
	if(info.st_uid != 0 || info.st_gid != 0)
		throw men + " not owned by root " + fileName;
	if(info.st_mode & ga_mask)
		throw men + " with insecure permissions (must be 0600/0700)" + fileName;
}

void Configuration::readConfigFile(){
	ConfigData configDefault;
	// Values used if parameter not present in configuration file
	configDefault["MAXTIME"] = "1200";
	configDefault["MAXFILESIZE"] = "64 Mb";
	configDefault["MAXMEMORY"] = "2000 Mb";
	configDefault["MAXPROCESSES"] = "500";
	configDefault["MIN_PRISONER_UGID"] = "10000";
	configDefault["MAX_PRISONER_UGID"] = "20000";
	configDefault["JAILPATH"] = "/jail";
	configDefault["CONTROLPATH"] = "/var/vpl-jail-system";
	configDefault["TASK_ONLY_FROM"] = "";
	configDefault["INTERFACE"] = "";
	configDefault["PORT"] = "80";
	configDefault["SECURE_PORT"] = "443";
	configDefault["URLPATH"] = "/";
	configDefault["ENVPATH"] = Util::getEnv("PATH");
	configDefault["LOGLEVEL"] = "0";
	configDefault["FAIL2BAN"] = "0";
	configDefault["SSL_CIPHER_LIST"] = "";
	configDefault["SSL_CIPHER_SUITES"] = "";
	configDefault["HSTS_MAX_AGE"] = "";
	configDefault["SSL_CERT_FILE"] = "/etc/vpl/cert.pem";
	configDefault["SSL_KEY_FILE"] = "/etc/vpl/key.pem";
	configDefault["USE_CGROUP"] = "false";
	configDefault["REQUEST_MAX_SIZE"] = "128 Mb";
	configDefault["RESULT_MAX_SIZE"] = "32 Kb";
	ConfigData data = ConfigurationFile::readConfiguration(configPath, configDefault);
	minPrisoner = atoi(data["MIN_PRISONER_UGID"].c_str());
	if(minPrisoner < JAIL_MIN_PRISONER_UID)
		throw "Incorrect MIN_PRISONER_UGID config value" + data["MIN_PRISONER_UGID"];
	maxPrisoner = atoi(data["MAX_PRISONER_UGID"].c_str());
	if(maxPrisoner > JAIL_MAX_PRISONER_UID)
		throw "Incorrect MAX_PRISONER_UGID config value" + data["MAX_PRISONER_UGID"];
	if(minPrisoner>maxPrisoner || minPrisoner<JAIL_MIN_PRISONER_UID || maxPrisoner>JAIL_MAX_PRISONER_UID)
		throw "Incorrect config file, prisoner uid inconsistency";
	jailPath = data["JAILPATH"];
	if(!Util::correctPath(jailPath))
		throw "Incorrect Jail"+jailPath;
	jailPath = jailPath[0] == '/' ? jailPath : "/" + jailPath; //Add absolute path
	if(jailPath == "/")
		throw "Jail path can NOT be set to /";
	controlPath = data["CONTROLPATH"];
	controlPath = controlPath[0] == '/' ? controlPath : "/" + controlPath; //Add absolute path
	if(!Util::correctPath(controlPath))
		throw "Incorrect control path "+controlPath;
	jailLimits.maxtime = atoi(data["MAXTIME"].c_str());
	jailLimits.maxfilesize = Util::memSizeToBytesl(data["MAXFILESIZE"]);
	jailLimits.maxmemory = Util::memSizeToBytesl(data["MAXMEMORY"]);
	jailLimits.maxprocesses = atoi(data["MAXPROCESSES"].c_str());
	istringstream iss(data["TASK_ONLY_FROM"]);
	for (string ipnet; iss >> ipnet; ) {
		taskOnlyFrom.push_back(ipnet);
		syslog(LOG_INFO,"TASK_ONLY_FROM found IP/net %s", ipnet.c_str());
	}
	interface = data["INTERFACE"];
	port = atoi(data["PORT"].c_str());
	sport = atoi(data["SECURE_PORT"].c_str());
	URLPath = data["URLPATH"];
	cleanPATH = data["ENVPATH"];
	logLevel = atoi(data["LOGLEVEL"].c_str());
	fail2ban = atoi(data["FAIL2BAN"].c_str());
	SSLCipherList = data["SSL_CIPHER_LIST"];
	SSLCipherSuites = data["SSL_CIPHER_SUITES"];
	HSTSMaxAge = -1;
	if (port > 0 && data["HSTS_MAX_AGE"].size() > 0) {
		HSTSMaxAge = atoi(data["HSTS_MAX_AGE"].c_str());
	}
	SSLCertFile = data["SSL_CERT_FILE"];
	SSLKeyFile = data["SSL_KEY_FILE"];
	useCgroup = Util::toUppercase(data["USE_CGROUP"]) == "TRUE";
	requestMaxSize = Util::memSizeToBytesi(data["REQUEST_MAX_SIZE"]);
	resultMaxSize = Util::memSizeToBytesi(data["RESULT_MAX_SIZE"]);
}

Configuration::Configuration() {
	configPath = "/etc/vpl/vpl-jail-system.conf";
	checkConfigFile(configPath, "Config file");
	readConfigFile();
}

Configuration::Configuration(string path) {
	configPath = path;
	readConfigFile();
}
