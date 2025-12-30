/**
 * package:		Part of vpl-jail-system
 * copyright:
 * license:		GNU/GPL, see LICENSE.txt or http://www.gnu.org/licenses/gpl-3.0.html
 *
 **/

#include <string>
#include "util.h"
#include <syslog.h>
#include <iostream>
#include <map>
#include <vector>
#include <fstream>
#include "vplregex.h"
using namespace std;

/**
 * Interface designed for access cgroup management of CPU, memory and net resources
 * Please, note that some of the interface's setters will require the user
 * to be part of the cgroup that is being modified
 */
class Cgroup {
private:
	static string baseCgroupFileSystem;
	string cgroupDirectory;
	bool isV2;
	string groupName;
	string joinPath(const string &a, const string &b) const {
		if (a.empty()) return b;
		if (b.empty()) return a;
		if (a[a.size() - 1] == '/') return a + b;
		return a + "/" + b;
	}
	bool isCgroupV2Base(const string &base) const {
		// cgroup v2 has a unified hierarchy and always exposes cgroup.controllers at the mount root.
		return Util::fileExists(joinPath(base, "cgroup.controllers"), true);
	}
	string v1Path(const char *relative) const {
		return cgroupDirectory + relative;
	}
	string v2Path(const string &relative) const {
		return joinPath(cgroupDirectory, relative);
	}
	void ensureV2CgroupExists();
	void attachPidV2(int pid);
	static vplregex regUser;
	static vplregex regSystem;
	static vplregex regPeriods;
	static vplregex regThrottled;
	static vplregex regThrottledTime;
	static vplregex regCache;
	static vplregex regMem;
	static vplregex regMapped;
	static vplregex regFault;
	static vplregex regHierarchical;
	static vplregex regEth0;
	static vplregex regEth1;
	static vplregex regLo;
	static vplregex regOOM;
	static vplregex regUnder;
	static vplregex regKill;
	static vplregex regTrim;
	static const char* FILE_CPU_ACCT_STAT;
	static const char* FILE_CPU_USAGE;
	static const char* FILE_CPU_TASKS;
	static const char* FILE_CPU_STAT;
	static const char* FILE_NET_TASKS;
	static const char* FILE_PIDS_PROCS;
	static const char* FILE_MEM_TASKS;
	static const char* FILE_MEM_LIMIT;
	static const char* FILE_MEM_STAT;
	static const char* FILE_MEM_USAGE;
	static const char* FILE_MEM_OOM_CONTROL;
	string regFound(vplregex &reg, string input);

public:
	static void setBaseCgroupFileSystem(string _baseCgroupFileSystem){
		baseCgroupFileSystem = _baseCgroupFileSystem;
	}

	static string getBaseCgroupFileSystem(){
		return baseCgroupFileSystem;
	}

	Cgroup(string name){
		groupName = name;
		string base = Cgroup::getBaseCgroupFileSystem();
		isV2 = isCgroupV2Base(base);
		if (isV2) {
			// v2: /sys/fs/cgroup/<name>
			cgroupDirectory = joinPath(base, name);
		} else {
			// v1 (legacy layout used by fixtures and existing code)
			cgroupDirectory = joinPath(base, name) + "/";
		}
	}

	~Cgroup(){
		Logger::log(LOG_DEBUG, "Destructor called.");
	}

	map<string, int> getCPUAcctStat();
	long int getCPUUsage();
	map<string, int> getCPUStat();
	vector<int> getCPUProcs();
	vector<int> getPIDs();
	vector<int> getNetProcs();
	vector<int> getMemoryProcs();
	long int getMemoryLimitInBytes();
	map<string, long int> getMemoryStat();
	long int getMemoryUsageInBytes();
	map<string, int> getMemoryOOMControl();
	void setNetProcs(int pid);
	void setCPUProcs(int pid);
	void setMemoryProcs(int pid);
	void setMemoryLimitInBytes(long int bytes);
	
	/**
	 * Remove this cgroup from all controllers
	 * Note: All processes must be moved out of the cgroup before removal
	 * @throws exception if cgroup cannot be removed (e.g., processes still in it)
	 */
	void removeCgroup();
};
