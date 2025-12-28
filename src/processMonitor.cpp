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
#include <sys/prctl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include "lock.h"
#include "processMonitor.h"
#include "vpl-jail-server.h"
#include "cgroup.h"
/**
 * Return the number of prisoners in jail
 */
int processMonitor::requestsInProgress() {
	const string homeDir = Configuration::getConfiguration()->getJailPath() + "/home";
	const string pre = "p";
	int nprisoner;
	dirent *ent;
	DIR *dirfd = opendir(homeDir.c_str());
	if (dirfd == NULL) {
		Logger::log(LOG_ERR, "Can't open dir \"%s\": %m", homeDir.c_str());
		return 0;
	}
	while ((ent = readdir(dirfd)) != NULL) {
		const string name(ent->d_name);
		if (name.find(pre, 0) == 0) nprisoner++;
	}
	closedir(dirfd);
	return nprisoner;
}

/**
 * Root mutate to be prisoner
 */
void processMonitor::becomePrisoner(int prisoner) {
	if (setresgid(prisoner, prisoner, prisoner)!= 0) {
		throw HttpException(internalServerErrorCode,
				"I can't change to prisoner group", Util::itos(prisoner));
	}
	if (setresuid(prisoner, prisoner, prisoner) != 0) {
		throw HttpException(internalServerErrorCode,
				"I can't change to prisoner user", Util::itos(prisoner));
	}
	//Recheck not needed
	if (getuid() == 0 || geteuid() == 0)
		throw HttpException(internalServerErrorCode,
				"I can't change to prisoner user 2", Util::itos(prisoner));
	
	// Reject all unnecessary privileges and prevent privilege escalation
	// Set NO_NEW_PRIVS to prevent gaining privileges through execve
	if (prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0) != 0) {
		Logger::log(LOG_WARNING, "Failed to set NO_NEW_PRIVS: %m");
	}
	// Set dumpable to 0 for additional security (prevents ptrace attach)
	if (prctl(PR_SET_DUMPABLE, 0, 0, 0, 0) != 0) {
		Logger::log(LOG_WARNING, "Failed to set DUMPABLE to 0: %m");
	}
	// Prevent other processes from ptracing this one
	if (prctl(PR_SET_PTRACER, 0, 0, 0, 0) != 0) {
		Logger::log(LOG_WARNING, "Failed to set PTRACER to 0: %m");
	}
	
	Daemon::closeSockets();
	Logger::log(LOG_INFO, "change user to %d (privileges rejected)", prisoner);
}

/**
 * Root mutate to be prisoner
 */
void processMonitor::becomePrisoner() {
	becomePrisoner(prisoner);
}

int processMonitor::stopPrisonerProcess(bool soft) {
	return stopPrisonerProcess(prisoner, soft);
}

vector<pid_t> processMonitor::getPrisonerProcesses(int prisoner) {
	vector<pid_t> pids;
	DIR *dir = opendir("/proc");
	if (dir) {
		struct dirent *entry;
		while ((entry = readdir(dir))) {
			if (entry->d_type == DT_DIR) {
				pid_t pid = atoi(entry->d_name);
				if (pid > 0 && getProcessUID(pid) == prisoner) {
					pids.push_back(pid);
				}
			}
		}
		closedir(dir);
	}
	return pids;
}
/**
 * Stop prisoner process
 * @param prisoner Prisoner id
 * @param soft If true send SIGTERM else SIGKILL
 * @return number of processes killed or 0 if /proc not found
 */
int processMonitor::stopPrisonerProcess(int prisoner, bool soft) {
	if (prisoner > JAIL_MAX_PRISONER_UID || prisoner < JAIL_MIN_PRISONER_UID) {
		Logger::log(LOG_ERR, "Try to stop prisoner process with invalid prisoner id %d. Ignoring", prisoner);
		return 0;
	}
	const int sleepTime = 100000;
	Logger::log(LOG_INFO, "Sttoping prisoner process");
	vector<pid_t> pids = getPrisonerProcesses(prisoner);
	// Kill all process of prisoner
	for (size_t i = 0; i < pids.size(); i++) {
		if (getProcessUID(pids[i]) != prisoner) continue;
		if (soft) {
			kill(pids[i], SIGTERM);
		} else {
			kill(pids[i], SIGKILL);
		}
	}
	// Wait for process end
	usleep(sleepTime);
	for (size_t i = 0; i < pids.size(); i++) {
		waitpid(pids[i], NULL, WNOHANG);
	}
	// Kill using plan B
	pid_t pid = fork();
	if (pid == 0) { //new process
		becomePrisoner(prisoner);
		if (soft) {
			//To not stop at first signal
			signal(SIGTERM, catchSIGTERM);
			kill(-1, SIGTERM);
		} else {
			kill(-1, SIGKILL);
		}
		_exit(EXIT_SUCCESS);
	} else {
		for (int i = 0; i < 50 ; i++) {
			usleep(100000);
			if (pid == waitpid(pid, NULL, WNOHANG)) //Wait for pid end
				break;
		}
	}
	return pids.size();
}

/**
 * return prisoner relative home page
 */
string processMonitor::prisonerRelativeHomePath() {
	char buf[100];
	sprintf(buf, "/home/p%d", prisoner);
	return buf;
}

/**
 * return prisoner home page
 * if user root absolute path if prisoner relative to jail path
 */
string processMonitor::prisonerHomePath() {
	uid_t uid = getuid();
	if (uid != 0) {
		Logger::log(LOG_ERR, "Security problem? prisonerHomePath %d", uid);
		return prisonerRelativeHomePath();
	}
	return configuration->getJailPath() + prisonerRelativeHomePath();
}

/**
 * Select a prisoner id from minPrisoner to maxPrisoner
 * Create prisoner home dir
 */
void processMonitor::selectPrisoner() {
	uid_t firstPrisoner = configuration->getMinPrisoner();
	int const range = configuration->getMaxPrisoner() - firstPrisoner + 1;
	int const ntry = range;
	mode_t const controlDirMode = 0700;
	mode_t const homeDirMode = 0770; // non-prisoner owned; prisoner group access
	if (range < 100) {
		throw HttpException(internalServerErrorCode,
				"Prisoner range too small, minPrisoner + 100 < maxPrisoner");
	}
	if (configuration->getMinPrisoner() < JAIL_MIN_PRISONER_UID ||
		configuration->getMaxPrisoner() > JAIL_MAX_PRISONER_UID) {
		throw HttpException(internalServerErrorCode,
				"Prisoner range out of limits, minPrisoner");
	}
	Lock lock(configuration->getControlPath());

	for (int i = 0; i < ntry; i++) {
		setPrisonerID(firstPrisoner + abs(Util::random() + i) % range);
		string controlPath = getProcessControlPath();
		if ( mkdir(controlPath.c_str(), controlDirMode) == 0 ) {
			homePath = prisonerHomePath();
			if ( mkdir(homePath.c_str(), homeDirMode) == 0 ) {
				if ( chown(homePath.c_str(), configuration->getHomeDirOwnerUid(), prisoner) ) {
					Util::removeDir(homePath, 0, true);
					throw HttpException(internalServerErrorCode
							, "I can't change prisoner home dir \"" + homePath +
							  "\" to proper owner: " + strerror(errno));
				}
				if ( chmod(homePath.c_str(), homeDirMode) ) {
					Util::removeDir(homePath, 0, true);
					throw HttpException(internalServerErrorCode
							, "I can't change prisoner home dir \"" + homePath +
							  "\" to proper permissions: " + strerror(errno));
				}
				return;
			} else if ( errno == EEXIST) {
				Logger::log(LOG_ERR, "Exists home dir without control dir, removing ... %s", homePath.c_str());
				Util::removeDir(homePath, getPrisonerID(), true);
				Util::removeDir(configuration->getJailPath() + "/tmp", getPrisonerID(), false);
				rmdir(controlPath.c_str());
				continue;
			}
			throw HttpException(internalServerErrorCode,
							"I can't create prisoner home dir \"" + homePath +
							"\" : " + strerror(errno));
		} else if ( errno == EEXIST) {
			continue;
		}
		throw HttpException(internalServerErrorCode,
						    "I can't create prisoner control dir \"" + Util::itos(prisoner) +
							"\" : "+ strerror(errno));
	}
	throw HttpException(internalServerErrorCode,
	                      "I can't select prisoner home dir: free UID not found");
}

void processMonitor::writeInfo(ConfigData data) {
	string configFile = getProcessConfigFile();
	//Read current config and don't lost data not set now
	if (Util::fileExists(configFile)) {
		data = ConfigurationFile::readConfiguration(configFile, data);
	}
	data["MAXTIME"] = Util::itos(executionLimits.maxtime);
	data["MAXFILESIZE"] = Util::itos(executionLimits.maxfilesize);
	data["MAXMEMORY"] = Util::itos(executionLimits.maxmemory);
	data["MAXPROCESSES"] = Util::itos(executionLimits.maxprocesses);

	data["ADMINTICKET"] = adminticket;
	data["EXECUTIONTICKET"] = executionticket;
	data["MONITORTICKET"] = monitorticket;
	data["HTTPPASSTHROUGHTTICKET"] = httpPassthroughticket;
	data["LOCALWEBSERVER"] = localwebserver;

	data["STARTTIME"] = Util::itos(startTime);
	data["INTERACTIVE"] = interactive?"1":"0";
	data["LANG"] = lang;
	data["COMPILER_PID"] = Util::itos(compiler_pid);
	data["RUNNER_PID"] = Util::itos(runner_pid);
	data["MONITOR_PID"] = Util::itos(monitor_pid);
	ConfigurationFile::writeConfiguration(configFile, data);
}

ConfigData processMonitor::readInfo() {
	ConfigData data;
	data = ConfigurationFile::readConfiguration(getProcessConfigFile(), data);
	executionLimits.maxtime = atoi(data["MAXTIME"].c_str());
	executionLimits.maxfilesize = atoll(data["MAXFILESIZE"].c_str());
	executionLimits.maxmemory = atoll(data["MAXMEMORY"].c_str());
	executionLimits.maxprocesses = atoi(data["MAXPROCESSES"].c_str());

	adminticket = data["ADMINTICKET"];
	executionticket = data["EXECUTIONTICKET"];
	monitorticket = data["MONITORTICKET"];
	httpPassthroughticket = data["HTTPPASSTHROUGHTTICKET"];
	localwebserver = data["LOCALWEBSERVER"];

	startTime = atoi(data["STARTTIME"].c_str());
	interactive = atoi(data["INTERACTIVE"].c_str());
	lang = data["LANG"];
	compiler_pid = atoi(data["COMPILER_PID"].c_str());
	runner_pid = atoi(data["RUNNER_PID"].c_str());
	monitor_pid = atoi(data["MONITOR_PID"].c_str());
	return data;
}

processMonitor::processMonitor(string & adminticket, string & monitorticket, string & executionticket) {
	prisoner = -1; // Not selected
	configuration = Configuration::getConfiguration();
	security = admin;
	string cp = configuration->getControlPath();
	adminticket = getPartialTicket();
	monitorticket = getPartialTicket();
	executionticket = getPartialTicket();
	selectPrisoner();
	executionticket += getPartialTicket();
	monitorticket += getPartialTicket();
	adminticket += getPartialTicket();
	{
		Lock lock(cp);
		while (Util::fileExists(cp + "/" + monitorticket) ||
		       Util::fileExists(cp + "/" + executionticket) ||
			   Util::fileExists(cp + "/" + adminticket)) {
			adminticket = getPartialTicket();
			monitorticket = getPartialTicket();
			executionticket = getPartialTicket();
			executionticket += getPartialTicket();
			monitorticket += getPartialTicket();
			adminticket += getPartialTicket();
		}
		// Write tickets.
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
		this->httpPassthroughticket = "";
		startTime = time(NULL);
		interactive = false;
		compiler_pid = 0;
		monitor_pid = 0;
		runner_pid = 0;
		writeInfo();
	}
}
processMonitor::processMonitor(string & adminticket, string & executionticket) {
	prisoner = -1; // Not selected
	configuration = Configuration::getConfiguration();
	security = admin;
	string cp = configuration->getControlPath();
	adminticket = getPartialTicket();
	executionticket = getPartialTicket();
	selectPrisoner();
	executionticket += getPartialTicket();
	adminticket += getPartialTicket();
	{
		Lock lock(cp);
		while (Util::fileExists(cp + "/" + executionticket) ||
			   Util::fileExists(cp + "/" + adminticket)) {
			adminticket = getPartialTicket();
			executionticket = getPartialTicket();
			executionticket += getPartialTicket();
			adminticket += getPartialTicket();
		}
		// Write tickets.
		ConfigData data;
		data["USER_ID"] = Util::itos(prisoner);
		data["SECURITY"] = Util::itos(admin);
		ConfigurationFile::writeConfiguration(cp + "/" + adminticket, data);
		data["SECURITY"] = Util::itos(execute);
		ConfigurationFile::writeConfiguration(cp + "/" + executionticket, data);
		executionLimits.maxtime = 0;
		executionLimits.maxfilesize = 0;
		executionLimits.maxmemory = 0;
		executionLimits.maxprocesses = 0;
		this->adminticket = adminticket;
		this->monitorticket = "NO_MONITOR";
		this->executionticket = executionticket;
		this->httpPassthroughticket = "";
		startTime = time(NULL);
		interactive = true;
		compiler_pid = 0;
		monitor_pid = 0;
		runner_pid = 0;
		writeInfo();
	}
}

processMonitor::processMonitor(string ticket) {
	prisoner = -1; // Not selected
	configuration = Configuration::getConfiguration();
	Util::trimAndRemoveQuotes(ticket);
	regex_t reg;
	regcomp(&reg, "^[0-9]+$", REG_EXTENDED);
	regmatch_t match[1];
	int nomatch = regexec(&reg, ticket.c_str(), 1, match, 0);
	regfree(&reg);
	if (nomatch == 0) {
		Lock lock(configuration->getControlPath());
		string fileName = configuration->getControlPath() + "/" + ticket;
		if (Util::fileExists(fileName)) {
			ConfigData data;
			data["USER_ID"] = "0";
			data["SECURITY"] = "5"; //none
			data = ConfigurationFile::readConfiguration(fileName, data);
			if (data["USER_ID"] == "0" || data["SECURITY"] == "5") {
				throw "Ticket invalid: task configuration lost";
			}
			setPrisonerID(atoi(data["USER_ID"].c_str()));
			security = (securityLevel) atoi(data["SECURITY"].c_str());
			if (security == monitor || security == execute) {
				Util::deleteFile(fileName); // Remove tikect
			}
			string configFile = getProcessConfigFile();
			if (Util::fileExists(configFile)) {
				readInfo();
				if (security == monitor) {
					monitorize();
				}
				return;
			} else {
				throw "Ticket invalid: task configuration lost";
			}
		} else {
			throw "Ticket not found";
		}
	} else {
		throw "Ticket invalid format";
	}
}

bool processMonitor::FileExists(string name) {
	return Util::fileExists(getHomePath() + "/" + name);
}

bool processMonitor::controlFileExists(string name) {
	return Util::fileExists(getProcessControlPath(name));
}
/**
 * Returns file content of file in user home directory
 * @param name File name
 * @return File content
 */
string processMonitor::readFile(string name) {
	return Util::readFile(getHomePath() + "/" + name, false, getHomePath().size() + 1);
}

void processMonitor::writeFile(string name, const string &data) {
	string homePath = getHomePath();
	string fullName = homePath + "/" + name;
	bool isScript = name.size()>4 && name.substr(name.size()-3) == ".sh";
	if (isScript) { //Endline converted to linux
		string newdata = data;
		Util::removeCRs(newdata);
		Util::writeFile(fullName, newdata, getPrisonerID(), homePath.size() + 1);
	}else{
		Util::writeFile(fullName, data, getPrisonerID(), homePath.size() + 1);
	}
}

/**
 * Delete a file from prisoner home directory
 */
void processMonitor::deleteFile(string name) {
	Util::deleteFile(getHomePath() + "/" + name, getHomePath().size() + 1);
}

bool processMonitor::installScript(string to, string from) {
	if (Util::fileExists("/usr/sbin/vpl/" + from)) {
		string scriptCode = Util::readFile("/usr/sbin/vpl/" + from);
		Logger::log(LOG_DEBUG, "Installing %s in %s", from.c_str(), to.c_str());
		writeFile(to, scriptCode);
		return true;
	}
	return false;
}

bool processMonitor::isRunnig() {
	return getState() != stopped;
}

processState processMonitor::getState() {
	if ( ! Util::dirExists(getProcessControlPath())) return stopped;
	string fileName = getProcessConfigFile();
	{
		Lock lock(getProcessControlPath());
		if ( ! Util::fileExists(fileName))	return stopped;
		readInfo();
	}
	if (compiler_pid == 0) return starting;
	time_t currentTime = time(NULL);
	if (startTime > currentTime || startTime == 0 ) {
		Logger::log(LOG_ERR, "Internal error startTime bad %ld", startTime);
		return starting;
	}
	time_t elapsedTime = currentTime - startTime;
	time_t tlimit = startTime;
	tlimit += 2 * executionLimits.maxtime;
	tlimit += JAIL_HARVEST_TIMEOUT;
	if (tlimit < currentTime) {
		Logger::log(LOG_INFO, "Execution last timeout reached %ld. ", tlimit);
		cleanTask();
		return stopped;
	}
	bool aliveCompiler = Util::processExists(compiler_pid);
	if (aliveCompiler && runner_pid == 0) return compiling;
	if (monitor_pid == 0 && monitorticket != "NO_MONITOR" && elapsedTime > JAIL_MONITORSTART_TIMEOUT) {
		Logger::log(LOG_INFO, "Execution without monitor timeout reached %d. ", JAIL_MONITORSTART_TIMEOUT);
		cleanTask();
		return stopped;
	}
	if (runner_pid == 0) {
		if (monitor_pid == 0 && monitorticket == "NO_MONITOR" && elapsedTime > JAIL_MONITORSTART_TIMEOUT) {
			Logger::log(LOG_INFO, "Execution not started with no monitor, timeout reached %d. ", JAIL_MONITORSTART_TIMEOUT);
			cleanTask();
			return stopped;
		}
		if (interactive) return beforeRunning;
		if (controlFileExists("compilation")) return retrieve;
		Logger::log(LOG_INFO, "Execution stopped, not interactive, not runner, no compilation file");
		return stopped;
	}
	bool aliveRunner = Util::processExists(runner_pid);
	if (aliveRunner) return running;
	if (interactive) return stopped;
	if (controlFileExists("execution")) return retrieve;
	return stopped;
}

void processMonitor::setRunner() {
	if (security == monitor) return;
	Lock lock(getProcessControlPath());
	readInfo();
	runner_pid = getpid();
	writeInfo();
}
void processMonitor::setCompiler() {
	if (security == monitor) return;
	Lock lock(getProcessControlPath());
	readInfo();
	startTime = time(NULL);
	compiler_pid = getpid();
	writeInfo();
}


bool processMonitor::isMonitored() {
	if ( ! Util::dirExists(getProcessControlPath())) return false;
	Lock lock(getProcessControlPath());
	if ( monitor == 0 ) readInfo();
	if (this->monitorticket == "NO_MONITOR" && this->isRunnig()) {
		return true;
	}
	if ( monitor == 0 ) return false;
	return Util::processExists(monitor);
}

void processMonitor::monitorize() {
	if ( security != monitor ) return;
	Lock lock(getProcessControlPath());
	readInfo();
	if (monitor_pid != 0)
		throw string("Process already monitorized");
	monitor_pid = getpid();
	writeInfo();
}

void processMonitor::setExtraInfo(ExecutionLimits el, bool ri, string lang) {
	Lock lock(getProcessControlPath());
	readInfo();
	executionLimits = el;
	interactive = ri;
	this->lang = lang;
	writeInfo();
}

void processMonitor::limitResultSize(string &r) {
	if (r.size()
			>= static_cast<unsigned int>(configuration->getRequestMaxSize())) {
		string men = "\nThis output has been cut to " + Util::itos(configuration->getRequestMaxSize()/1024) + "Kb" +
				", its original size was " + Util::itos(r.size()/1024) + "Kb\n";
		r = men + r.substr(0, configuration->getRequestMaxSize()/2) + men + r.substr(r.size()-configuration->getRequestMaxSize()/2);
	}
}
void processMonitor::getResult(string &compilation, string &execution, bool &executed) {
	if (security != admin)
		throw HttpException(internalServerErrorCode, "Security: required admin ticket for request");
	if (isInteractive())
		throw HttpException(internalServerErrorCode, "Security: process in bad state");
	{
		string fileName;
		Lock lock(getProcessControlPath());
		compilation = "";
		execution = "";
		executed = false;
		fileName = getProcessControlPath("compilation");
		if (Util::fileExists(fileName)) {
			compilation = Util::readFile(fileName);
			limitResultSize(compilation);
			Util::deleteFile(fileName);
		}
		fileName = getProcessControlPath("execution");
		if ((executed = Util::fileExists(fileName))) {
			execution = Util::readFile(fileName);
			limitResultSize(execution);
			Util::deleteFile(fileName);
		}
		if (compilation.empty() && ! executed) {
			compilation = "The compilation process did not generate an executable nor error message.";
		}
	}
}

string processMonitor::getCompilation() {
	if (security != monitor)
		throw HttpException(internalServerErrorCode, "Security: required monitor ticket for getCompilation");
	{
		Lock lock(getProcessControlPath());
		string fileName = getProcessControlPath("compilation");
		if (Util::fileExists(fileName))
			return Util::readFile(fileName);
		return "";
	}
}

void processMonitor::setCompilationOutput(const string &compilation) {
	string fileName;
	Lock lock(getProcessControlPath());
	fileName = getProcessControlPath("compilation");
	if (Util::fileExists(fileName))
		throw "Compilation already saved";
	Util::writeFile(fileName, compilation);
}
void processMonitor::setExecutionOutput(const string &execution, bool executed) {
	string fileName;
	Lock lock(getProcessControlPath());
	fileName = getProcessControlPath("compilation");
	if ( ! Util::fileExists(fileName))
		throw "Compilation not saved";
	if (executed) {
		fileName = getProcessControlPath("execution");
		if (Util::fileExists(fileName))
			throw "Execution already saved";
		Util::writeFile(fileName, execution);
	}
}

string processMonitor::getHttpPassthroughTicket() {
	readInfo();
	if ( httpPassthroughticket == "" ) {
		string cp = configuration->getControlPath();
		{
			httpPassthroughticket = getPartialTicket() + getPartialTicket();
			Lock lock(cp);
			while (Util::fileExists(cp + "/" + httpPassthroughticket)) {
				httpPassthroughticket = getPartialTicket() + getPartialTicket();
			}
			//Write ticket
			ConfigData data;
			data["USER_ID"] = Util::itos(prisoner);
			data["SECURITY"] = Util::itos(httppassthrough);
			ConfigurationFile::writeConfiguration(cp + "/" + httpPassthroughticket, data);
			writeInfo();
		}
	}
	return httpPassthroughticket;
}

string processMonitor::getLocalWebServer() {
	if ( localwebserver == "" ) {
		readInfo();
		if (localwebserver == "" && FileExists(VPL_LOCALSERVERADDRESSFILE)) {
			localwebserver = readFile(VPL_LOCALSERVERADDRESSFILE);
			writeInfo();
		}
	}
	return localwebserver;
}

/**
 * remove a directory and its content
 * if force = true remove always
 * else remove files owned by prisoner
 * and complete directories owned by prisoner (all files and directories owns by prisoner or not)
 */
int processMonitor::removeDir(string dir, bool force) {
	return Util::removeDir(dir, prisoner, force);
}

/**
 * Remove prisoner files and stop prisoner process
 */

/**
 * Removes ticket file if set and exists
 */
void processMonitor::removeTicketFile(string ticket) {
	if ( ! ticket.empty() ) {
		string controlPath = Configuration::getConfiguration()->getControlPath();
		Util::deleteFile(controlPath + "/" + ticket);
	}
}

/**
 * Remove task home dir and monitor control files
 */
void processMonitor::cleanTask() {
	const int maxretry = 100;
	const int sleepTime = 100000;
	const int userid = getPrisonerID();
	if (userid == -1) {
		return; //No prisoner to clean
	}
	Logger::log(LOG_INFO, "Cleaning task");
	stopPrisonerProcess(userid, true);
	usleep(sleepTime);
	int retry = 0;
	int processes = 0;
	while( (processes = stopPrisonerProcess(userid, false)) > 0
	        && retry < maxretry) {
		retry++;
		usleep(sleepTime);
	}
	if (processes > 0) {
		vector<pid_t> pids = getPrisonerProcesses(userid);
		for (size_t i = 0; i < pids.size(); i++) {
			string processName;
			string processPath;
			Util::getProcessName(pids[i], processName, processPath);
			Logger::log(LOG_ERR, "Can't stop prisoner UID = %d process = %d '%s' '%s'",
				        userid, pids[i], processName.c_str(), processPath.c_str());
		}
	}
	cleanPrisonerFiles("p" + Util::itos(userid));
}

bool processMonitor::isOutOfMemory() {
	if (executionLimits.maxmemory <= 0) return false;
	
	// Use cgroup-based monitoring if enabled
	if (configuration->getUseCGroup()) {
		try {
			string cgroupName = "p" + Util::itos(prisoner);
			Cgroup cgroup(cgroupName);
			
			// Check OOM status directly from cgroup
			map<string, int> oomControl = cgroup.getMemoryOOMControl();
			if (oomControl["under_oom"] > 0) {
				Logger::log(LOG_DEBUG, "Cgroup reports under_oom condition");
				return true;
			}
			
			// Check current memory usage vs limit
			long int usage = cgroup.getMemoryUsageInBytes();
			Logger::log(LOG_DEBUG, "Cgroup memory usage: %ld / %lld", usage, executionLimits.maxmemory);
			return usage >= executionLimits.maxmemory;
		} catch (const std::exception &e) {
			Logger::log(LOG_DEBUG, "Failed to read cgroup memory status: %s, falling back to /proc", e.what());
		} catch (...) {
			Logger::log(LOG_DEBUG, "Failed to read cgroup memory status, falling back to /proc");
		}
	}
	
	// Fallback to /proc-based monitoring
	return executionLimits.maxmemory < getMemoryUsed();
}

string processMonitor::getMemoryLimit() {
	if (executionLimits.maxmemory == 0) return "unlimited";
	return Util::itos(executionLimits.maxmemory/(1024*1024)) + "MiB";
}

/**
 * Get process UID from /proc
 * @param pid Process id
 * @return UID or -1 if not found
 */
int processMonitor::getProcessUID(pid_t  pid) {
	static bool init = false;
	static regex_t reg_uid;
	if (!init) {
		regcomp(&reg_uid, ".*^Uid:[ \t]+([0-9]+)", REG_EXTENDED|REG_ICASE|REG_NEWLINE);
		init = true;
	}
	const int matchSize = 2;
	regmatch_t match[matchSize];
	const string statusFile = "/proc/" + Util::itos(pid) + "/status";
	string status = Util::readFile(statusFile, false);
	int nomatch = regexec(&reg_uid, status.c_str(), matchSize, match, 0);
	if (nomatch == 0) {
		string UIDF = status.substr(match[1].rm_so, match[1].rm_eo - match[1].rm_so);
		return Util::atoi(UIDF);
	}
	return -1;
}

long long processMonitor::getMemoryUsed() {
	// Use cgroup-based monitoring if enabled
	if (configuration->getUseCGroup()) {
		try {
			string cgroupName = "p" + Util::itos(prisoner);
			Cgroup cgroup(cgroupName);
			long int usage = cgroup.getMemoryUsageInBytes();
			Logger::log(LOG_DEBUG, "Cgroup memory usage: %ld bytes", usage);
			return usage;
		} catch (const std::exception &e) {
			Logger::log(LOG_DEBUG, "Failed to read cgroup memory usage: %s, falling back to /proc", e.what());
		} catch (...) {
			Logger::log(LOG_DEBUG, "Failed to read cgroup memory usage, falling back to /proc");
		}
	}
	
	// Fallback to /proc scanning
	string dir = "/proc";
	dirent *ent;
	DIR *dirfd = opendir(dir.c_str());
	if (dirfd == NULL) {
		Logger::log(LOG_ERR, "Can't open \"/proc\" dir: %m");
		return INT_MAX; //
	}
	static bool init = false;
	static regex_t reg_uid;
	static regex_t reg_mem;
	if (!init) {
		regcomp(&reg_uid, ".*^Uid:[ \t]+([0-9]+)", REG_EXTENDED|REG_ICASE|REG_NEWLINE);
		regcomp(&reg_mem, ".*^VmHWM:[ \t]+([0-9]+)[ \t]+(.*)", REG_EXTENDED|REG_ICASE|REG_NEWLINE);
		init = true;
	}
	const int matchSize = 3;
	regmatch_t match[matchSize];
	long long s = 0;
	while ((ent = readdir(dirfd)) != NULL) {
		const string name(ent->d_name);
		const pid_t PID = Util::atoi(name);
		if (Util::itos(PID) != name ) continue;
		const string statusFile = dir + "/" + name + "/status";
		string status = Util::readFile(statusFile, false);
		int nomatch = regexec(&reg_uid, status.c_str(), matchSize, match, 0);
		if (nomatch == 0) {
			string UIDF = status.substr(match[1].rm_so, match[1].rm_eo - match[1].rm_so);
			if (Util::atoi(UIDF) == (int) prisoner) {
				nomatch = regexec(&reg_mem, status.c_str(), matchSize, match, 0);
				if (nomatch == 0) {
					string MEM = status.substr(match[1].rm_so, match[1].rm_eo - match[1].rm_so);
					string MUL = status.substr(match[2].rm_so, match[2].rm_eo - match[2].rm_so);
					const int onek = 1024;
					int mul = onek; //Default kB 1024
					if (MUL == "mB") mul *= onek;
					s += Util::atol(MEM) * mul;
				}
			}
		}
	}
	closedir(dirfd);
	return s;
}

void processMonitor::freeWatchDog() {
	//TODO check process running based on /proc
	//looking for inconsistencies
}

vector<string> processMonitor::getPrisonersFromDir(string dir) {
	static const vplregex reguserdir("^p[0-9]+$");
	vplregmatch match;
	vector<string> prisoners;
	dirent *ent;
	DIR *dirfd = opendir(dir.c_str());
	if (dirfd == NULL) {
		Logger::log(LOG_ERR, "Can't open dir \"%s\": %m", dir.c_str());
		return prisoners;
	}
	while((ent = readdir(dirfd)) != NULL) {
		if (ent->d_type == DT_DIR) {
			const string name(ent->d_name);
			if (reguserdir.search(name, match)) {
				prisoners.push_back(name);
			}
		}
	}
	closedir(dirfd);
	return prisoners;
}

void processMonitor::cleanPrisonerFiles(string pdir) {
	Logger::log(LOG_INFO, "Cleaning prisoner files");
	const string controlDir = Configuration::getConfiguration()->getControlPath();
	const string jailPath = Configuration::getConfiguration()->getJailPath();
	const string phome = jailPath + "/home/" + pdir;
	const string tmpDir = jailPath + "/tmp";
	const string configDir = controlDir + "/" + pdir;
	const string configFile = configDir + "/" + "config";
	ConfigData data;
	int uid = Util::atoi(pdir.substr(1));
	Lock lock(controlDir);
	if (Util::fileExists(configFile)) {
		try {
			data = ConfigurationFile::readConfiguration(configFile, data);
			removeTicketFile( data["ADMINTICKET"] );
			removeTicketFile( data["EXECUTIONTICKET"] );
			removeTicketFile( data["MONITORTICKET"] );
			removeTicketFile( data["HTTPPASSTHROUGHTTICKET"] );
		} catch(...) {}
	}
	try {
		Util::removeDir(phome, uid, true);
	} catch(...) {}
	try {
		Util::removeDir(tmpDir, uid, false);
	}catch(...) {}
	try {
		Util::removeDir(configDir, 0, true);
	}catch(...) {}
}

void processMonitor::cleanZombieTasks() {
	Logger::log(LOG_INFO, "Cleaning zombie tasks");
	const string controlDir = Configuration::getConfiguration()->getControlPath();
	const string homeDir = Configuration::getConfiguration()->getJailPath() + "/home";
	vector<string> homes = getPrisonersFromDir(homeDir);
	vector<string> tasks = getPrisonersFromDir(controlDir);
	for (size_t i = 0; i < tasks.size(); i++) {
		ConfigData data;
		Logger::log(LOG_INFO, "Cleaning zombie tasks: checking(1) %s", tasks[i].c_str());
		string configDir = controlDir + "/" + tasks[i];
		string configFile = configDir + "/" + "config";
		struct stat statbuf;
		try {
			stat(configDir.c_str(), &statbuf);
			if ( statbuf.st_mtime + JAIL_MONITORSTART_TIMEOUT < time(NULL) ) {
				if ( ! Util::dirExists(homeDir + "/" + tasks[i]) ) {
					cleanPrisonerFiles(tasks[i]);
				} else {
					data = ConfigurationFile::readConfiguration(configFile, data);
					processMonitor pm(data["ADMINTICKET"]);
					pm.getState();	
				}
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
	for (size_t i = 0; i < homes.size(); i++) {
		Logger::log(LOG_INFO, "Cleaning zombie tasks: checking(2) %s", homes[i].c_str());
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
