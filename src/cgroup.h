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
	static string baseCgroupFileSystem;
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
	string regFound(regex_t reg, string input);

public:
	static void setBaseCgroupFileSystem(string _baseCgroupFileSystem){
		baseCgroupFileSystem = _baseCgroupFileSystem;
	}
	static string getBaseCgroupFileSystem(){
		return baseCgroupFileSystem;
	}
	Cgroup(string name){
		cgroupDirectory =  Cgroup::getBaseCgroupFileSystem() + "/" + name + "/";
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

	map<string, int> getCPUAcctStat();
	int getCPUCfsPeriod();
	int getCPUCfsQuota();
	long int getCPUUsage();
	map<string, int> getCPUStat();
	int getNotify();
	string getReleaseAgent();
	string getCPUTasks();
	int getCloneChildren();
	string getMemoryTasks();
	int getMemCloneChildren();
	bool getMemoryMigrate();
	long int getMemoryLimitInBytes();
	map<string, int> getMemoryStat();
	int getMemSwappiness();
	long int getMemoryUsageInBytes();
	int getMemNotify();
	bool setMemHardwall();
	string getMemReleaseAgent();
	long int getMemoryFailCnt();
	int getMemoryMoveChargeImmigrate();
	int getMemoryUseHierarchy();
	int getMemoryOOMControl();

	void setCPUCfsPeriod(int period);
	void setCPUCfsQuota(int quota);
	void setCPUCloneChildren(bool flag);
	void setCPUProcs(int pid);
	void setCPUShares(int share);	
	void setCPUNotify(bool flag);
	void setCPUReleaseAgentPath(string path);
	void setCPUs(string cpus);
	void setMems(string nodes);
	void setMemoryMigrate(bool flag);
	void setCPUExclusive(bool flag);
	void setMemExclusive(bool flag);
	void setMemHardwall(bool flag);
	void setMemoryPressure(bool flag);
	void setMemorySpreadPage(bool flag);
	void setMemorySpreadSlab(bool flag);
	void setSchedLoadBalance(bool flag);
	
};


