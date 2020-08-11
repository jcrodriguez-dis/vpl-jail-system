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
using namespace std;

class Cgroup {
private:
	static string baseCgroupFilesystem;
	string cgroupDirectory;
	regex_t regUser;
	regex_t regSystem;
	regex_t regPeriods;
	regex_t regThrottled;
	regex_t regThrottledTime;
	regex_t regCache;
	regex_t regMem;
	regex_t regMapped;
	regex_t regFault;
	regex_t regHierarchical;

public:
	static void setBaseCgroupFilesystem(string fs);
	Cgroup(string name){
		cgroupDirectory = baseCgroupFilesystem + "/" + name + "/";
		regcomp(&regUser,"^user ([0-9]+)$",REG_EXTENDED);
		regcomp(&regSystem,"^system ([0-9]+)$",REG_EXTENDED);
		regcomp(&regPeriods,"^nr_periods ([0-9]+)$",REG_EXTENDED);
		regcomp(&regThrottled,"^nr_throttled ([0-9]+)$",REG_EXTENDED);
		regcomp(&regThrottledTime,"^throttled_time ([0-9]+)$",REG_EXTENDED);
		regcomp(&regCache,"^cache ([0-9]+)$",REG_EXTENDED);
		regcomp(&regMem,"^shmem ([0-9]+)$",REG_EXTENDED);
		regcomp(&regMapped,"^mapped_file ([0-9]+)$",REG_EXTENDED);
		regcomp(&regHierarchical,"^hierarchical_memory_limit ([0-9]+)$",REG_EXTENDED);

	}

	string regFound(regex_t reg, string input);
	map<string, int> getCPUAcctStat();
	long int getCPUCfsPeriod();
	long int getCPUCfsQuota();
	long int getCPUUsage();
	map<string, int> getCPUStat();
	int getNotify();
	string getReleaseAgent();
	string getCPUTasks();
	int getCloneChildren();
	string getMemoryTasks();
	int getMemCloneChildren();
	long int getMemoryLimitInBytes();
	map<string, int> getMemoryStat();
	int getMemSwappiness();
	long int getMemoryUsageInBytes();
	int getMemNotify();
	string getMemReleaseAgent();
	long int getMemoryFailCnt();
	int getMemoryMoveChargeImmigrate();
	int getMemoryUseHierarchy();
	int getMemoryOOMControl();
	map<string, long int> getBlkioThrottleReadBps();
	map<string, int> getBlkioThrottleReadIops();
	map<string, int> getBlkioThrottleWriteBps();
	map<string, int> getBlkioThrottleWriteIops();

	void setCPUCfsPeriod(int period);
	void setCPUCfsQuota(int quota);
	void setCPUCloneChildren(bool flag);
	void setCPUProcs(int pid);
	void setCPUShares(int share);	
	void setCPUNotify(int flag);
	void setCPUReleaseAgentPath(int path);
	void setCPUs(int cpus);
	void setMems(int nodes);
	void setMemoryMigrate(int flag);
	void setCPUExclusive(int flag);
	void setMemExclusive(int flag);
	void setMemHardwall(int flag);
	void setMemoryPressure(int flag);
	void setMemorySpreadPage(int flag);
	void setSchedLoadBalance(int flag);
	
	
	
	
};
