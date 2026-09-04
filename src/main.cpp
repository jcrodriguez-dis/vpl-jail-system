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
#include "cgroup.h"
#include "log.h"

using namespace std;

#ifndef TEST

class Main {
public:
	const static char * pidFile;
	const static char * readyFile;
	/**
	 * @brief Detach from controlling terminal (classic double-fork).
	 *
	 * Writes a PID file to `/run/vpl-jail-server.pid`.
	 */
	static void daemonize(){
		pid_t child_pid = fork();
		if(child_pid < 0) {
			Logger::log(LOG_EMERG, "daemonize() => fork() fail (child_pid < 0)");
			exit(EXIT_FAILURE);
		}
		if(child_pid > 0) _exit(EXIT_SUCCESS); //grandparent exit
		if(setsid() < 0) {
			Logger::log(LOG_EMERG, "daemonize() => (setsid() < 0)");
			exit(EXIT_FAILURE);
		}
		pid_t grandchild_pid = fork();
		if(grandchild_pid < 0) {
			Logger::log(LOG_EMERG, "daemonize() => fork() fail (grandchild_pid < 0)");
			exit(EXIT_FAILURE);
		}
		if(grandchild_pid > 0) _exit(EXIT_SUCCESS); //parent exit
		// Redirect stdin/stdout/stderr to /dev/null so terminal closure
		// does not send SIGHUP or cause EIO errors.
		int devnull = open("/dev/null", O_RDWR);
		if (devnull >= 0) {
			dup2(devnull, STDIN_FILENO);
			dup2(devnull, STDOUT_FILENO);
			dup2(devnull, STDERR_FILENO);
			if (devnull > STDERR_FILENO) close(devnull);
		}
		// Avoid keeping any directory's filesystem busy.
		if (chdir("/") != 0) {
			Logger::log(LOG_WARNING, "daemonize() => chdir(\"/\") fail: %s", strerror(errno));
		}
		Util::writeFile(pidFile, std::to_string((int)getpid()));
		if (Util::fileExists(pidFile, true)) {
			Logger::log(LOG_INFO, "daemonize() => PID file written to %s", pidFile);
		} else {
			Logger::log(LOG_EMERG, "daemonize() => PID file NOT written to %s", pidFile);
			exit(EXIT_FAILURE);
		}
		FILE *fd = fopen(pidFile, "w");
		if(fd == nullptr) {
			Logger::log(LOG_EMERG, "daemonize() => fopen(\"%s\") fail: %s", pidFile, strerror(errno));
			exit(EXIT_FAILURE);
		}
		if (fprintf(fd, "%d", (int)getpid()) < 0) {
			Logger::log(LOG_EMERG, "daemonize() => fprintf(\"%s\") fail: %s", pidFile, strerror(errno));
			exit(EXIT_FAILURE);
		}
		if (fclose(fd) != 0) {
			Logger::log(LOG_EMERG, "daemonize() => fclose(\"%s\") fail: %s", pidFile, strerror(errno));
			exit(EXIT_FAILURE);
		}
	}

	/**
	 * @brief Run in the foreground but still create a PID file.
	 *
	 * In some environments (e.g., Docker) `setsid()` may fail; this is tolerated.
	 */
	static void foreground(){
		setsid(); // NOTE: fail in Docker.
		FILE *fd = fopen("/run/vpl-jail-server.pid", "w");
		if(fd == nullptr) {
			Logger::log(LOG_EMERG, "foreground() => fopen(\"/run/vpl-jail-server.pid\") fail: %s", strerror(errno));
			exit(EXIT_FAILURE);
		}
		if (fprintf(fd, "%d", (int)getpid()) < 0) {
			Logger::log(LOG_EMERG, "foreground() => fprintf(\"/run/vpl-jail-server.pid\") fail: %s", strerror(errno));
			exit(EXIT_FAILURE);
		}
		if (fclose(fd) != 0) {
			Logger::log(LOG_EMERG, "foreground() => fclose(\"/run/vpl-jail-server.pid\") fail: %s", strerror(errno));
			exit(EXIT_FAILURE);
		}
	}
	/**
	 * @brief Wait for the ready file notification to Jail be ready
	 */
	static void waitReadyFile(){
		const int maxWaitSeconds = 30;
		const int tickTime = 100000; // 0.1 second
		const int maxticks = maxWaitSeconds * 1000000 / tickTime;
		for (int ticks = 0; ticks < maxticks; ticks++) {
			if (Util::fileExists(readyFile, true)) {
				Logger::log(LOG_INFO, "Jail ready for use at %s", Configuration::getConfiguration()->getJailPath().c_str());
				return;
			}
			Util::sleep(tickTime); // 0.1 second
		}
		Logger::log(LOG_EMERG, "Jail not ready after %d seconds", maxWaitSeconds);
		exit(EXIT_FAILURE);
	}

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
	 * Report cgroup availability at startup, as the per task setup runs in a child
	 * process whose log goes to syslog only.
	 */
	static void checkCgroupAvailability(const Configuration *conf) {
		if (!conf->getUseCGroup()) {
			Logger::log(LOG_NOTICE, "Cgroup use is disabled by configuration (USE_CGROUP)");
			return;
		}
		if (Cgroup::isAvailable()) {
			Logger::log(LOG_NOTICE, "Cgroup %s available at '%s'",
					Cgroup::isCgroupV2() ? "v2" : "v1", Cgroup::getBaseCgroupFileSystem().c_str());
		} else {
			Logger::log(LOG_WARNING, "Cgroup hierarchy unavailable: tasks will run without"
					" cgroup memory control. Mount a writable cgroup filesystem"
					" (e.g. run the container with --privileged or"
					" -v /sys/fs/cgroup:/sys/fs/cgroup:rw)");
		}
	}
};
const char * Main::pidFile = "/run/vpl-jail-server.pid";
const char * Main::readyFile = "/run/vpl-jail-server.ready";

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
	Logger::setLogLevel(LOG_ERR, true); // Default log level for early messages
	Configuration *conf = Configuration::getConfiguration();
	Logger::setLogLevel(conf->getLogLevel(), true);
	bool containerAutoDetected = Main::detectContainerMode(conf->getJailPath());
	bool runningInContainer = containerAutoDetected || containerByArgument;
	string startupMessage = "Server running";
	if (foreground){
		startupMessage += " in foreground mode";
	} else {
		startupMessage += " as daemon";
	}
	if (runningInContainer) {
		startupMessage += " inside a container";
		if (containerAutoDetected && containerByArgument) {
			startupMessage += " (container by argument and auto-detection)";
		} else if (containerAutoDetected) {
			startupMessage += " (auto-detected)";
		} else {
			startupMessage += " (argument not confirmed)";
		}
	} else {
		startupMessage += " on host system";
		if (Main::isInContainer()) {
			startupMessage += " (container detected)";
		}
	}
	Logger::log(LOG_NOTICE, "%s", startupMessage.c_str());
	if (conf->getJailPath() == "" && ! runningInContainer) {
		Logger::log(LOG_EMERG, "Jail directory root \"/\" but not running in container");
		exit(1);
	}
	if (conf->getJailPath() != "" && runningInContainer) {
		Logger::log(LOG_EMERG, "Running in container but Jail directory not root \"/\"");
		exit(1);
	}
	if (foreground) {
		Main::foreground();
	} else {
		Main::daemonize();
	}
	// Wait for Jail creation end
	Logger::log(LOG_NOTICE, "Waiting for the Jail File System to be ready ...");
	Main::waitReadyFile();
	if (conf->getLogLevel() >= LOG_INFO) {
		conf->readConfigFile(); // Reread configuration file to show values in log
	}
	conf->findWritableDirsInJail(); // Find writable dirs inside jail
	conf->setInContainer(runningInContainer || foreground);
	Main::checkSecurityConfiguration(conf);
	Main::checkPrivateNetwork();
	Main::checkCgroupAvailability(conf);
	int exitStatus = static_cast<int>(internalError);
	try{
		Daemon* runner = Daemon::getRunner();
		Logger::log(LOG_NOTICE, "VPL Jail Server %s started", Util::version());
		Logger::setLogLevel(conf->getLogLevel(), foreground);
		runner->loop();
		exitStatus = EXIT_SUCCESS;
	}
	catch(HttpException &exception) {
		Logger::setLogLevel(conf->getLogLevel(), true);
		Logger::log(LOG_CRIT, "%s", exception.getLog().c_str());
		exitStatus=static_cast<int>(httpError);
	}
	catch(const string &me) {
		Logger::setLogLevel(conf->getLogLevel(), true);
		exitStatus = EXIT_FAILURE;
		Logger::log(LOG_CRIT, "%s", me.c_str());
	}
	catch(const char * const me) {
		Logger::setLogLevel(conf->getLogLevel(), true);
		exitStatus = EXIT_FAILURE;
		Logger::log(LOG_CRIT, "%s",me);
	}
	catch(std::exception &e) {
		Logger::setLogLevel(conf->getLogLevel(), true);
		exitStatus = EXIT_FAILURE;
		Logger::log(LOG_CRIT, "Unexpected exception: %s %s:%d", e.what(), __FILE__, __LINE__);
	}
	catch(...){
		Logger::setLogLevel(conf->getLogLevel(), true);
		exitStatus = EXIT_FAILURE;
		Logger::log(LOG_CRIT, "Unexpected exception %s:%d", __FILE__, __LINE__);
	}
	exit(exitStatus);
}
#endif
