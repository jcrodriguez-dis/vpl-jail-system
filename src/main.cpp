/**
 * package:		Part of vpl-jail-system
 * copyright:	Copyright (C) 2009 Juan Carlos Rodríguez-del-Pino. All rights reserved.
 * license:		GNU/GPL, see LICENSE.txt or http://www.gnu.org/licenses/gpl-3.0.html
 **/

#include <exception>
#include <fstream>
#include <unistd.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
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
 * Check if an IPv4 address is in a private network range.
 * Private ranges: 10.0.0.0/8, 172.16.0.0/12, 192.168.0.0/16, 127.0.0.0/8
 */
static bool isPrivateIPv4(const struct in_addr &addr) {
	uint32_t ip = ntohl(addr.s_addr);
	if ((ip >> 24) == 10) return true;          // 10.0.0.0/8
	if ((ip >> 20) == 0xAC1) return true;       // 172.16.0.0/12
	if ((ip >> 16) == 0xC0A8) return true;      // 192.168.0.0/16
	if ((ip >> 24) == 127) return true;          // 127.0.0.0/8
	return false;
}

/**
 * Warn if the server is not directly accessible from the internet
 * (all non-loopback interfaces have private IP addresses).
 * Unavoidable warnings.
 */
static void checkPrivateNetwork() {
	struct ifaddrs *ifaddr = nullptr;
	if (getifaddrs(&ifaddr) == -1) {
		Logger::log(LOG_WARNING, "Unable to get network interfaces: %m");
		return;
	}
	bool hasPublicIP = false;
	bool hasNonLoopback = false;
	for (struct ifaddrs *ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
		if (ifa->ifa_addr == nullptr || ifa->ifa_addr->sa_family != AF_INET) {
			continue;
		}
		struct sockaddr_in *sa = reinterpret_cast<struct sockaddr_in *>(ifa->ifa_addr);
		if (ntohl(sa->sin_addr.s_addr) >> 24 == 127) {
			continue; // Skip loopback
		}
		hasNonLoopback = true;
		if (!isPrivateIPv4(sa->sin_addr)) {
			hasPublicIP = true;
			break;
		}
	}
	freeifaddrs(ifaddr);
	if (hasNonLoopback && !hasPublicIP) {
		int logLevel = Logger::getLogLevel();
		if (logLevel < LOG_WARNING) {
			Logger::setLogLevel(LOG_WARNING, Logger::isForeground());
		}

		const char *warningMessage = "Server is on a private network and not directly accessible from the internet.";
		Logger::log(LOG_WARNING, warningMessage);
	}
}

/**
 * Check security configuration for URLPATH and TASK_ONLY_FROM.
 * Unavoidable warnings.
 */
static void checkSecurityConfiguration(const Configuration *conf) {
	bool insecure = false;
	int logLevel = Logger::getLogLevel();
	bool foreground = Logger::isForeground();
	bool logNeedAdapted = logLevel < LOG_WARNING || !foreground;
	if (logNeedAdapted) {
		Logger::setLogLevel(LOG_WARNING, true);
	}
	if (conf->getURLPath() == "/") {
		const char *warningMessage = "URLPATH is not set. URLPATH acts as a password to accept tasks.\n"
									 "Without it, any Moodle or similar system can send task requests to this server.";
		Logger::log(LOG_WARNING, warningMessage);
		insecure = true;
	}
	if (conf->getURLPath() != "/" && conf->getPort() != 0) {
		const char *warningMessage = "URLPATH is set but unciphered port (HTTP) is enabled.\n"
									 "URLPATH is transmitted over HTTP from Moodle to this server"
									 " and can be intercepted on the network.";
		Logger::log(LOG_WARNING, warningMessage);
		insecure = true;
	}
	if (insecure &&conf->getTaskOnlyFrom().size() == 0) {
		const char *warningMessage = "TASK_ONLY_FROM is not set. Task requests are not restricted to servers\n"
									 "with predefined (allowed) IP addresses.";
		Logger::log(LOG_WARNING, warningMessage);
	}
	if (insecure) {
		const char *warningMessage = "Improve the security configuration: set URLPATH, TASK_ONLY_FROM, and PORT=0.\n"
									 "See manual at https://vpl.dis.ulpgc.es/";
		Logger::log(LOG_WARNING, warningMessage);
	}
	if (logNeedAdapted) {
		Logger::setLogLevel(logLevel, foreground);
	}
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
	checkSecurityConfiguration(conf);
	checkPrivateNetwork();
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
		Logger::log(LOG_CRIT, "%s", exception.getLog().c_str());
		exitStatus=static_cast<int>(httpError);
	}
	catch(const string &me) {
		exitStatus = EXIT_FAILURE;
		Logger::log(LOG_CRIT, "%s", me.c_str());
	}
	catch(const char * const me) {
		exitStatus = EXIT_FAILURE;
		Logger::log(LOG_CRIT, "%s",me);
	}
	catch(std::exception &e) {
		exitStatus = EXIT_FAILURE;
		Logger::log(LOG_CRIT, "Unexpected exception: %s %s:%d", e.what(), __FILE__, __LINE__);
	}
	catch(...){
		exitStatus = EXIT_FAILURE;
		Logger::log(LOG_CRIT, "Unexpected exception %s:%d", __FILE__, __LINE__);
	}
	exit(exitStatus);
}
#endif
