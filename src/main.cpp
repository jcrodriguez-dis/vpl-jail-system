/**
 * package:		Part of vpl-jail-system
 * copyright:	Copyright (C) 2009 Juan Carlos Rodríguez-del-Pino. All rights reserved.
 * license:		GNU/GPL, see LICENSE.txt or http://www.gnu.org/licenses/gpl-3.0.html
 **/

#include <exception>
#include <fstream>
#include <unistd.h>
#include "vpl-jail-server.h"
#include "configuration.h"
#include "log.h"

using namespace std;

#ifndef TEST

static bool isInContainer() {
	// Check /proc/mounts for overlay on /
	{
		ifstream mounts("/proc/mounts");
		string line;
		while (getline(mounts, line)) {
			if (line.rfind("overlay / overlay", 0) == 0) {
				return true;
			}
		}
	}
	// Check /proc/self/mountinfo as fallback (cgroup v2 systems)
	{
		ifstream mountinfo("/proc/self/mountinfo");
		string line;
		while (getline(mountinfo, line)) {
			if (line.find(" / / ") != string::npos &&
			    line.find("overlay") != string::npos) {
				return true;
			}
		}
	}
	// Check cgroup membership for container runtimes
	{
		ifstream cgroup("/proc/1/cgroup");
		string line;
		while (getline(cgroup, line)) {
			if (line.find("docker") != string::npos ||
			    line.find("lxc") != string::npos ||
			    line.find("kubepods") != string::npos ||
			    line.find("libpod") != string::npos) {
				return true;
			}
		}
	}
	// Check PID 1 name: in a container it is the app, not init/systemd
	{
		ifstream sched("/proc/1/sched");
		string line;
		if (getline(sched, line)) {
			if (line.find("init") == string::npos &&
			    line.find("systemd") == string::npos) {
				return true;
			}
		}
	}
	// Docker-specific marker file
	if (access("/.dockerenv", F_OK) == 0) {
		return true;
	}
	return false;
}

/**
 * Detect if the process is running inside a container used as jail (JAILPATH="/").
 */
static bool detectContainerMode(const string &jailPath) {
	if (jailPath != "") {
		return false; // explicit non-root jail: never container mode
	}
	return isInContainer();
}

/**
 * main accept command line "foreground" to go non-daemon run.
 */
int main(int const argc, const char ** const argv) {
	bool foreground = false;
	bool containerByArgument = false;
	for (int i = 1; i < argc; i++) {
		string arg = argv[i];
		if (arg == "foreground") {
			foreground = true;
		}
		if (arg ==  "in_container") {
			containerByArgument = true;
		}
		if (arg ==  "version" || arg == "-version") {
			cout << Util::version() <<endl;
			exit(EXIT_SUCCESS);
		}
	}
	Logger::setLogLevel(LOG_ERR, foreground);
	Configuration *conf = Configuration::getConfiguration();
	Logger::setLogLevel(conf->getLogLevel(), foreground);
	if ( conf->getLogLevel() >= LOG_INFO) {
		conf->readConfigFile(); // Reread configuration file to show values in log
		conf->foundWritableDirsInJail(); // Reread writable dirs to show values in log
	}
	bool containerAutoDetected = detectContainerMode(conf->getJailPath());
	bool runningInContainer = containerAutoDetected || containerByArgument;

	cout << "Server running";
	if (foreground){
		cout << " in foreground mode";
	} else {
		cout << " as daemon";
	}
	if (runningInContainer) {
		cout << " inside a container";
		if (containerAutoDetected && containerByArgument) {
			cout << " (container by argument and auto-detection)";
		} else if (containerAutoDetected) {
			cout << " (auto-detected)";
		} else {
			cout << " (argument not confirmed)";
		}
	} else {
		cout << " on host system";
		if (isInContainer()) {
			cout << " (container detected)";
		}
	}
	cout << endl;
	if (conf->getJailPath() == "" && ! runningInContainer) {
		Logger::log(LOG_EMERG, "Jail directory root \"/\" but not running in container");
		exit(1);
	}
	if (conf->getJailPath() != "" && runningInContainer) {
		Logger::log(LOG_EMERG, "Running in container but Jail directory not root \"/\"");
		exit(1);
	}
	conf->setInContainer(runningInContainer || foreground);
	int exitStatus = static_cast<int>(internalError);
	try{
		Daemon* runner = Daemon::getRunner();
		if (foreground) {
			runner->foreground();
		} else {
			runner->daemonize();
		}
		Logger::log(LOG_INFO, "VPL Jail Server %s started", Util::version());
		runner->loop();
		exitStatus = EXIT_SUCCESS;
	}
	catch(HttpException &exception) {
		Logger::log(LOG_WARNING, "%s", exception.getLog().c_str());
		exitStatus=static_cast<int>(httpError);
	}
	catch(const string &me) {
		exitStatus = EXIT_FAILURE;
		Logger::log(LOG_WARNING, "%s", me.c_str());
	}
	catch(const char * const me) {
		exitStatus = EXIT_FAILURE;
		Logger::log(LOG_WARNING, "%s",me);
	}
	catch(std::exception &e) {
		exitStatus = EXIT_FAILURE;
		Logger::log(LOG_EMERG, "Unexpected exception: %s %s:%d", e.what(), __FILE__, __LINE__);
	}
	catch(...){
		exitStatus = EXIT_FAILURE;
		Logger::log(LOG_EMERG, "Unexpected exception %s:%d", __FILE__, __LINE__);
	}
	exit(exitStatus);
}
#endif
