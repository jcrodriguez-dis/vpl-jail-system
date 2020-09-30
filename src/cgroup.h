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
#include <fstream>
using namespace std;

/**
 * Interface designed for a better management of CPU, memory and net resources
 * Please, note that some of the interface's setters will require the user
 * to be part of the cgroup that is being modified
 */
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
	static regex regTrim;
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
