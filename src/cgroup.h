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
	regex_t regEth0;
	regex_t regEth1;
	regex_t regLo;
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
		regcomp(&regUser,"^user [0-9]+",REG_EXTENDED); // [0-9]+$
		regcomp(&regSystem,"[\n]system [0-9]+",REG_EXTENDED);
		regcomp(&regPeriods,"^nr_periods [0-9]+",REG_EXTENDED);
		regcomp(&regThrottled,"[\n]nr_throttled [0-9]+",REG_EXTENDED);
		regcomp(&regThrottledTime,"[\n]throttled_time [0-9]+",REG_EXTENDED);
		regcomp(&regCache,"^cache [0-9]+",REG_EXTENDED);
		regcomp(&regMem,"[\n]shmem [0-9]+",REG_EXTENDED);
		regcomp(&regMapped,"[\n]mapped_file [0-9]+",REG_EXTENDED);
		regcomp(&regFault,"[\n]pgfault [0-9]+",REG_EXTENDED);
		regcomp(&regHierarchical,"[\n]hierarchical_memory_limit [0-9]+",REG_EXTENDED);
		regcomp(&regEth0,"^eth0 ([0-9]+)$",REG_EXTENDED);
		regcomp(&regEth1,"^eth1 ([0-9]+)$",REG_EXTENDED);
		regcomp(&regLo,"^lo ([0-9]+)$",REG_EXTENDED);
	}

	~Cgroup(){
		syslog(LOG_DEBUG, "Destructor called.");
	}

	map<string, int> getCPUAcctStat();
	int getCPUUsage();
	map<string, int> getCPUStat();
	int getNotify();
	string getReleaseAgent();
	string getCPUTasks();
	int getNetPrioID();
	vector<int> getPIDs();
	map<string, int> getNetPrioMap();
	string getMemoryTasks();
	long int getMemoryLimitInBytes();
	map<string, int> getMemoryStat();
	long int getMemoryUsageInBytes();
	int getMemNotify();
	string getMemReleaseAgent();
	int getMemoryOOMControl();

	string setNetPrioMap();
	void setCPUCloneChildren(bool flag);
	void setCPUProcs(int pid);
	void setCPUShares(int share);	
	void setCPUNotify(bool flag);
	void setCPUReleaseAgentPath(string path);
	void setCPUs(string cpus);
	void setMemExclusive(bool flag);
	void setMemoryPressure(bool flag);
	void setMemorySpreadPage(bool flag);
	
};


