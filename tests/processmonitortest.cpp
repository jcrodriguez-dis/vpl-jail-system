/**
 * Tests for completed batch result retention.
 */
#include "basetest.h"
#include <cassert>
#include <climits>
#include <fstream>
#include <utime.h>
#include "../src/processMonitor.h"
#include "../src/jail_limits.h"

class ProcessMonitorTestConfiguration: public Configuration {
	Configuration *previous;
public:
	ProcessMonitorTestConfiguration(const string &path): Configuration(path) {
		previous = singlenton;
		singlenton = this;
	}
	~ProcessMonitorTestConfiguration() {
		singlenton = previous;
	}
};

class ProcessMonitorTest: public BaseTest {
	string root;
	string control;
	string task;
	const string ticket = "12345678901234567890";

	void prepare(bool interactive, bool executed, bool alive, time_t startTime) {
		assert(mkdir(task.c_str(), 0700) == 0);
		ConfigData data;
		data["USER_ID"] = "50001";
		data["SECURITY"] = "0";
		ConfigurationFile::writeConfiguration(control + "/" + ticket, data);
		data.clear();
		data["ADMINTICKET"] = ticket;
		data["EXECUTIONTICKET"] = "22345678901234567890";
		data["MONITORTICKET"] = "NO_MONITOR";
		data["STARTTIME"] = Util::itos(startTime);
		data["MAXTIME"] = "1";
		data["INTERACTIVE"] = interactive ? "1" : "0";
		data["COMPILER_PID"] = Util::itos(alive && !executed ? getpid() : INT_MAX);
		data["RUNNER_PID"] = executed ? Util::itos(alive ? getpid() : INT_MAX) : "0";
		data["MONITOR_PID"] = "0";
		ConfigurationFile::writeConfiguration(task + "/config", data);
		// Refuse to clean a UID already used by a real process in the test namespace.
		assert(processMonitor(ticket).getPrisonerProcesses(50001).empty());
		ofstream(task + "/compilation") << "compile-output";
		if (executed) ofstream(task + "/execution") << "execution-output";
	}

	void removeTask() {
		Util::removeDir(task, 0, true);
		unlink((control + "/" + ticket).c_str());
	}

	void testCompleted(bool executed) {
		prepare(false, executed, false, time(NULL) - 100);
		processMonitor pm(ticket);
		assert(pm.getState() == retrieve);
		assert(pm.getState() == retrieve); // Polling must not renew or discard the result.
		string compilation, execution;
		bool ran;
		pm.getResult(compilation, execution, ran);
		assert(compilation == "compile-output");
		assert(ran == executed);
		assert(execution == (executed ? "execution-output" : ""));
		removeTask();
	}

	void testConsumption(bool executed) {
		prepare(false, executed, false, time(NULL));
		processMonitor pm(ticket);
		string compilation, execution;
		bool ran;
		pm.getResult(compilation, execution, ran);
		assert(ran == executed);
		assert(pm.getState() == stopped); // startTime is current, so stopped means getresult consumed the output.
		assert(!Util::fileExists(task + "/compilation"));
		assert(!Util::fileExists(task + "/execution"));
		removeTask();
	}

	void testExpiry(bool executed) {
		prepare(false, executed, false, time(NULL));
		struct utimbuf times;
		times.actime = times.modtime = time(NULL) - JAIL_HARVEST_TIMEOUT - 2;
		assert(utime((task + (executed ? "/execution" : "/compilation")).c_str(), &times) == 0);
		processMonitor pm(ticket);
		assert(pm.getState() == stopped);
		removeTask();
	}

	void testActiveAndInteractive() {
		prepare(false, false, true, time(NULL));
		assert(processMonitor(ticket).getState() == compiling);
		removeTask();
		prepare(false, true, true, time(NULL));
		assert(processMonitor(ticket).getState() == running);
		removeTask();
		prepare(true, true, false, time(NULL));
		assert(processMonitor(ticket).getState() == stopped);
		removeTask();
		prepare(false, true, true, time(NULL) - 100);
		assert(processMonitor(ticket).getState() == stopped); // Active tasks retain their timeout.
		removeTask();
		prepare(false, true, false, time(NULL));
		processMonitor pm(ticket);
		pm.cleanTask();
		assert(pm.getState() == stopped); // Explicit Stop takes priority over harvest.
		removeTask();
	}

public:
	string name() { return "Process monitor result retention"; }
	void launch() {
		char path[] = "/tmp/vpl-process-monitor-test-XXXXXX";
		assert(mkdtemp(path) != NULL);
		root = path;
		control = root + "/control";
		task = control + "/p50001";
		assert(mkdir(control.c_str(), 0700) == 0);
		ofstream config(root + "/configuration");
		config << "JAILPATH=" << root << "\nCONTROLPATH=" << control
				<< "\nMIN_PRISONER_UGID=50000\nMAX_PRISONER_UGID=50200\n";
		config.close();
		const char *oldControlPath = getenv("VPL_JAIL_CONTROLPATH");
		string savedControlPath = oldControlPath == NULL ? "" : oldControlPath;
		assert(setenv("VPL_JAIL_CONTROLPATH", control.c_str(), 1) == 0);
		{
			ProcessMonitorTestConfiguration configuration(root + "/configuration");
			testCompleted(true);
			testCompleted(false);
			testConsumption(true);
			testConsumption(false);
			testExpiry(true);
			testExpiry(false);
			testActiveAndInteractive();
		}
		if (oldControlPath == NULL) unsetenv("VPL_JAIL_CONTROLPATH");
		else setenv("VPL_JAIL_CONTROLPATH", savedControlPath.c_str(), 1);
		Util::removeDir(root, 0, true);
	}
};

ProcessMonitorTest processmonitortest;
