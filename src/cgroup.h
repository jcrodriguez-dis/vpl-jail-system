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
	string regFound(vplregex &reg, string input);

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
	int getCPUNotify();
	string getCPUReleaseAgent();
	vector<int> getCPUProcs();
	int getNetPrioID();
	vector<int> getPIDs();
	map<string, int> getNetPrioMap();
	int getNetNotify();
	string getNetReleaseAgent();
	vector<int> getNetProcs();
	vector<int> getMemoryProcs();
	long int getMemoryLimitInBytes();
	map<string, long int> getMemoryStat();
	long int getMemoryUsageInBytes();
	int getMemNotify();
	string getMemReleaseAgent();
	map<string, int> getMemoryOOMControl();

	void setNetPrioMap(string interface);
	void setNetNotify(bool flag);
	void setNetReleaseAgent(string path);
	void setNetProcs(int pid);
	void setCPUProcs(int pid);
	void setCPUNotify(bool flag);
	void setCPUReleaseAgentPath(string path);
	void setMemoryProcs(int pid);
	void setMemoryLimitInBytes(long int bytes);
	void setMemNotify(bool flag);
	void setMemReleaseAgentPath(string path);

};
