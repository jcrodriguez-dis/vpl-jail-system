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
#include <regex>

using namespace std;

class Cgroup {
private:
	static string baseCgroupFileSystem;
	string cgroupDirectory;
	static regex regUser;
	static regex regSystem;
	static regex regPeriods;
	static regex regThrottled;
	static regex regThrottledTime;
	static regex regCache;
	static regex regMem;
	static regex regMapped;
	static regex regFault;
	static regex regHierarchical;
	static regex regEth0;
	static regex regEth1;
	static regex regLo;
	static regex regOOM;
	static regex regUnder;
	static regex regKill;
	string regFound(regex &reg, string input);

public:
	static void setBaseCgroupFileSystem(string _baseCgroupFileSystem){
		baseCgroupFileSystem = _baseCgroupFileSystem;
	}
	static string getBaseCgroupFileSystem(){
		return baseCgroupFileSystem;
	}
	Cgroup(string name){
		cgroupDirectory =  Cgroup::getBaseCgroupFileSystem() + "/" + name + "/";

	}

	~Cgroup(){
		syslog(LOG_DEBUG, "Destructor called.");
	}

	map<string, int> getCPUAcctStat();
	long int getCPUUsage();
	map<string, int> getCPUStat();
	int getNotify();
	string getReleaseAgent();
	string getCPUTasks();
	int getNetPrioID();
	vector<int> getPIDs();
	map<string, int> getNetPrioMap();
	vector<int> getMemoryTasks();
	long int getMemoryLimitInBytes();
	map<string, long int> getMemoryStat();
	long int getMemoryUsageInBytes(); //
	int getMemNotify(); //
	string getMemReleaseAgent(); //
	map<string, int> getMemoryOOMControl(); //

	void setNetPrioMap(string interface);
	void setCPUCloneChildren(bool flag);
	void setCPUProcs(int pid);
	void setCPUNotify(bool flag);
	void setCPUReleaseAgentPath(string path);
	void setCPUs(string cpus);
	

};


