/**
 * package:		Part of vpl-jail-system
 * copyright:	Copyright (C) 2014 Juan Carlos Rodríguez-del-Pino. All rights reserved.
 * license:		GNU/GPL, see LICENSE.txt or http://www.gnu.org/licenses/gpl-3.0.html
 **/
#include <sys/syslog.h>
#include <climits>
#include <sstream>
#include "configuration.h"
#include "jail_limits.h"
#include "util.h"

Configuration *Configuration::singlenton = 0;

string Configuration::generateCleanPATH(string path, string dirtyPATH) {
	string dir;
	size_t pos=0;
	size_t found;
	string clean;
	while(true) {
		if((found=dirtyPATH.find(':', pos)) != string::npos) {
			dir = dirtyPATH.substr(pos,found-pos);
		}else{
			dir = dirtyPATH.substr(pos);
		}
		if(Util::dirExistsFollowingSymLink(path+dir)) {
			if(clean.size()>0){
				clean += ':';
			}
			clean += dir;
		} else {
			Logger::log(LOG_INFO, "Path removed: %s", dir.c_str());
		}
		if(found == string::npos) return clean;
		pos=found+1;
	}
	return "";
}

void Configuration::checkConfigFile(string fileName, string men) {
	const int ga_mask = 077;
	struct stat info;
	if(lstat(fileName.c_str(), &info))
		throw men + " not found " + fileName;
	if(info.st_uid != 0 || info.st_gid != 0)
		throw men + " not owned by root " + fileName;
	if(info.st_mode & ga_mask)
		throw men + " with insecure permissions (must be 0600/0700)" + fileName;
}

void Configuration::readEnvironmentConfigVars(ConfigData &data) {
	const char *ENV_PREFIX = "VPL_JAIL_";
	extern char **environ;
	for(ConfigData::iterator i=data.begin(); i != data.end(); i++) {
		string parameter = i->first;
		string new_value = Util::getEnv(ENV_PREFIX + parameter);
        if (new_value.length() > 0) {
			data[parameter] = new_value;
			Logger::log(LOG_INFO, "Using parameter from environment var %s%s = %s",
			            ENV_PREFIX, parameter.c_str(), new_value.c_str());
		}
    }
	for (char **environArray = environ; *environArray; environArray++) {
		string name = Util::getEnvNameFromRaw(*environArray);
		name = Util::toUppercase(name);
		if (name.find(ENV_PREFIX) == 0 && data.find(name.substr(strlen(ENV_PREFIX))) == data.end()) {
			Logger::log(LOG_ERR, "Warning: Unknown environment var %s", *environArray);
		}
	}
}

void Configuration::readConfigFile() {
	ConfigData configDefault;
	// Values used if parameter not present in configuration file
	configDefault["PORT"] = "80";
	configDefault["SECURE_PORT"] = "443";
	configDefault["INTERFACE"] = "";
	configDefault["SSL_CIPHER_LIST"] = "";
	configDefault["SSL_CIPHER_SUITES"] = "";
	configDefault["HSTS_MAX_AGE"] = "";
	configDefault["SSL_CERT_FILE"] = "/etc/vpl/cert.pem";
	configDefault["SSL_KEY_FILE"] = "/etc/vpl/key.pem";
	configDefault["CERTBOT_WEBROOT_PATH"] = "";

	configDefault["URLPATH"] = "/";
	configDefault["TASK_ONLY_FROM"] = "";
	configDefault["ALLOWSUID"] = "false";
	configDefault["FAIL2BAN"] = "0";
	configDefault["FIREWALL"] = "0";
	configDefault["LOGLEVEL"] = "0";
	configDefault["JAILPATH"] = "/jail";
	configDefault["CONTROLPATH"] = "/var/vpl-jail-system";
	configDefault["USETMPFS"] = "";
	configDefault["HOMESIZE"] = "";
	configDefault["SHMSIZE"] = "";

	configDefault["MAXTIME"] = "1200";
	configDefault["MAXFILESIZE"] = "64 Mb";
	configDefault["MAXMEMORY"] = "2000 Mb";
	configDefault["MAXPROCESSES"] = "500";
	configDefault["REQUEST_MAX_SIZE"] = "128 Mb";
	configDefault["RESULT_MAX_SIZE"] = "1 Mb";

	configDefault["MIN_PRISONER_UGID"] = "10000";
	configDefault["MAX_PRISONER_UGID"] = "20000";
	configDefault["ENVPATH"] = Util::getEnv("PATH");
	configDefault["USE_CGROUP"] = "true";
	configDefault["USE_NAMESPACE"] = "true";

	ConfigData data = ConfigurationFile::readConfiguration(configPath, configDefault);
	// Check unkown parameters
	for(ConfigData::iterator i = data.begin(); i != data.end(); i++) {
		string param = i->first;
		param = Util::toUppercase(param);
		if(configDefault.find(param) == data.end())
			Logger::log(LOG_ERR,"Error: Unknown config param %s", param.c_str());
	}

	readEnvironmentConfigVars(data);
	inContainer = false;
	minPrisoner = atoi(data["MIN_PRISONER_UGID"].c_str());
	if(minPrisoner < JAIL_MIN_PRISONER_UID || minPrisoner > JAIL_MAX_PRISONER_UID)
		throw "Incorrect MIN_PRISONER_UGID config value" + data["MIN_PRISONER_UGID"];
	maxPrisoner = atoi(data["MAX_PRISONER_UGID"].c_str());
	if(maxPrisoner > JAIL_MAX_PRISONER_UID || maxPrisoner < JAIL_MIN_PRISONER_UID)
		throw "Incorrect MAX_PRISONER_UGID config value" + data["MAX_PRISONER_UGID"];
	if(minPrisoner >= maxPrisoner || maxPrisoner - minPrisoner < 100)
		throw "Incorrect config file, prisoner uid inconsistency, min range 100";
	
	jailPath = data["JAILPATH"];
	jailPath = jailPath[0] == '/' ? jailPath : "/" + jailPath; //Add absolute path
	if (jailPath == "/") {
		jailPath = "";
		data["JAILPATH"] = jailPath;
	}
	if(jailPath != "" and ! Util::correctPath(jailPath))
		throw "Incorrect Jail path " + jailPath;
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
		Logger::log(LOG_INFO, "TASK_ONLY_FROM found IP/net %s", ipnet.c_str());
	}
	interface = data["INTERFACE"];
	port = atoi(data["PORT"].c_str());
	sport = atoi(data["SECURE_PORT"].c_str());
	URLPath = data["URLPATH"];
	if (URLPath.size() > 0 && URLPath[0] != '/') {
		URLPath = "/" + URLPath;
	}
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
	useNamespace = Util::toUppercase(data["USE_NAMESPACE"]) == "TRUE";
	requestMaxSize = Util::memSizeToBytesi(data["REQUEST_MAX_SIZE"]);
	resultMaxSize = Util::memSizeToBytesi(data["RESULT_MAX_SIZE"]);
	certbotWebrootPath = data["CERTBOT_WEBROOT_PATH"];
}

Configuration::Configuration() {
	configPath = "/etc/vpl/vpl-jail-system.conf";
	checkConfigFile(configPath, "Config file");
	readConfigFile();
	foundWritableDirsInJail();
}

Configuration::Configuration(string path) {
	configPath = path;
	readConfigFile();
	foundWritableDirsInJail();
}

/**
 * Returns writable directories inside directory dirPath
 */
vector<string> Configuration::getWritableDirsInDir(const string &dirPath) {
	vector<string> writableDirs;
	DIR *dir = opendir(dirPath.c_str());
	if (dir == NULL) {
		Logger::log(LOG_ERR, "Error opening directory %s to find writable dirs", dirPath.c_str());
		return writableDirs;
	}
	uid_t minPrisoner = this->getMinPrisoner();
	uid_t maxPrisoner = this->getMaxPrisoner();
	struct dirent *entry;
	while ((entry = readdir(dir)) != NULL) {
		string entryName = entry->d_name;
		if (entryName == "." || entryName == "..") {
			continue;
		}
		string fullPath = dirPath + "/" + entryName;
		struct stat info;
		if (lstat(fullPath.c_str(), &info) != 0) {
			Logger::log(LOG_ERR, "Error stating file %s to find writable dirs", fullPath.c_str());
			continue;
		}
		if (S_ISDIR(info.st_mode)) {
			// Check if writable by prisoners
			if ((info.st_mode & S_IWOTH) ||
			    ((info.st_mode & S_IWGRP) && info.st_gid >= minPrisoner && info.st_gid <= maxPrisoner) ||
			    ((info.st_mode & S_IWUSR) && info.st_uid >= minPrisoner && info.st_uid <= maxPrisoner)) {
				string jailRelativePath = fullPath.substr(jailPath.size());
				writableDirs.push_back(jailRelativePath);
				Logger::log(LOG_INFO, "Found writable dir in jail: %s", jailRelativePath.c_str());
				continue;
			}
			// Recurse into subdirectory
			vector<string> subDirs = getWritableDirsInDir(fullPath);
			writableDirs.insert(writableDirs.end(), subDirs.begin(), subDirs.end());
		}
	}
	closedir(dir);
	return writableDirs;
}

void Configuration::foundWritableDirsInJail() {
	writableDirsInJail = getWritableDirsInDir(jailPath);
	Logger::log(LOG_INFO, "Total writable dirs found in jail: %lu", (long unsigned int)writableDirsInJail.size());
	for (size_t i = 0; i < writableDirsInJail.size(); i++) {
		Logger::log(LOG_INFO, "Writable dir in jail: %s", writableDirsInJail[i].c_str());
	}
}
