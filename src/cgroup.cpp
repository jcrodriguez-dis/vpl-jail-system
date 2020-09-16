/**
 * package:		Part of vpl-jail-system
 * copyright:
 * license:		GNU/GPL, see LICENSE.txt or http://www.gnu.org/licenses/gpl-3.0.html
 **/

#include "cgroup.h"

using namespace std;

string Cgroup::baseCgroupFileSystem="/sys/fs/cgroup";
regex Cgroup::regUser("(^|\\n)user ([0-9]+)(\\n|$)");
regex Cgroup::regSystem("(^|\\n)system ([0-9]+)(\\n|$)");
regex Cgroup::regPeriods("(^|\\n)nr_periods ([0-9]+)(\\n|$)");
regex Cgroup::regThrottled("(^|\\n)nr_throttled ([0-9]+)(\\n|$)");
regex Cgroup::regThrottledTime("(^|\\n)throttled_time ([0-9]+)(\\n|$)");
regex Cgroup::regCache("(^|\\n)cache ([0-9]+)(\\n|$)");
regex Cgroup::regMem("(^|\\n)shmem ([0-9]+)(\\n|$)");
regex Cgroup::regMapped("(^|\\n)mapped_file ([0-9]+)(\\n|$)");
regex Cgroup::regFault("(^|\\n)pgfault ([0-9]+)(\\n|$)");
regex Cgroup::regHierarchical("(^|\\n)hierarchical_memory_limit ([0-9]+)(\\n|$)");
regex Cgroup::regEth0("(^|\\n)eth0 ([0-9]+)(\\n|$)");
regex Cgroup::regEth1("(^|\\n)eth1 ([0-9]+)(\\n|$)");
regex Cgroup::regLo("(^|\\n)lo ([0-9]+)(\\n|$)");
regex Cgroup::regOOM("(^|\\n)oom_kill_disable ([0-9]+)(\\n|$)");
regex Cgroup::regUnder("(^|\\n)under_oom ([0-9]+)(\\n|$)");
regex Cgroup::regKill("(^|\\n)oom_kill ([0-9]+)(\\n|$)");
string Cgroup::regFound(regex &reg, string input){
	smatch found;
	string match;
	bool matchFound = regex_search(input,found,reg);
	if (matchFound) {
		match = found[2];
	}
	return match;
}

map<string, int> Cgroup::getCPUAcctStat(){
	map<string, int> cpuStat;
	string stat;
	syslog(LOG_DEBUG, "Reading from the file '%s'", (cgroupDirectory + "cpu/cpuacct.stat").c_str());
	stat = Util::readFile((cgroupDirectory + "cpu/cpuacct.stat").c_str());

	string sUser = regFound(regUser, stat);
	string sSystem = regFound(regSystem, stat);

	cpuStat["user"] = Util::atoi(sUser);
	cpuStat["system"] = Util::atoi(sSystem);
	return cpuStat;
}


long int Cgroup::getCPUUsage(){
	string path = cgroupDirectory + "cpu/cpuacct.usage";
	syslog(LOG_DEBUG, "Reading from the file '%s'", path.c_str());
	return Util::atol(Util::readFile((cgroupDirectory + "cpu/cpuacct.usage")));
}

int Cgroup::getNotify(){
	string path = cgroupDirectory + "cpu/notify_on_release";
	syslog(LOG_DEBUG, "Reading from the file '%s'", path.c_str());
	return Util::atoi(Util::readFile((cgroupDirectory + "cpu/notify_on_release").c_str()));
}

string Cgroup::getReleaseAgent(){
	string path = cgroupDirectory + "cpu/release_agent";
	syslog(LOG_DEBUG, "Reading from the file '%s'", path.c_str());
	return Util::readFile((cgroupDirectory + "cpu/release_agent").c_str());
}

string Cgroup::getCPUTasks(){
	string path = cgroupDirectory + "cpu/tasks";
	syslog(LOG_DEBUG, "Reading from the file '%s'", path.c_str());
	return Util::readFile((cgroupDirectory + "cpu/tasks").c_str());
}

map<string, int> Cgroup::getCPUStat(){
	map<string, int> cpuStat;
	string path = cgroupDirectory + "cpu/cpu.stat";
	syslog(LOG_DEBUG, "Reading from the file '%s'", path.c_str());
	string stat = Util::readFile((cgroupDirectory + "cpu/cpu.stat").c_str());

	string nrPeriods = regFound(regPeriods, stat);
	string nrThrottled = regFound(regThrottled, stat);
	string throttledTime = regFound(regThrottledTime, stat);

	cpuStat["nr_periods"] = Util::atoi(nrPeriods);
	cpuStat["nr_throttled"] = Util::atoi(nrThrottled);
	cpuStat["throttled_time"] = Util::atoi(throttledTime);
	return cpuStat;
}

int Cgroup::getNetPrioID(){
	string path = cgroupDirectory + "net_prio/net_prio.prioidx";
	syslog(LOG_DEBUG, "Reading from the file '%s'", path.c_str());
	return Util::atoi(Util::readFile((cgroupDirectory + "net_prio/net_prio.prioidx").c_str()));
}

vector<int> Cgroup::getPIDs(){
	string path = cgroupDirectory + "pids/cgroup.procs";
	syslog(LOG_DEBUG, "Reading from the file '%s'", path.c_str());
	string file = Util::readFile((cgroupDirectory + "pids/cgroup.procs").c_str());
	vector<int> pids;
	size_t pos = 0;
	size_t ini = 0;
	while((pos = file.find('\n', ini)) != string::npos){
		pids.push_back(Util::atoi(file.substr(ini, pos)));
		ini = pos + 1;
	}
	return pids;
}

map<string, int> Cgroup::getNetPrioMap(){
	map<string, int> netPrioMap;
	string path = cgroupDirectory + "net_prio/net_prio.ifpriomap";
	syslog(LOG_DEBUG, "Reading from the file '%s'", path.c_str());
	string stat = Util::readFile((cgroupDirectory + "net_prio/net_prio.ifpriomap").c_str());
	string eth0 = regFound(regEth0, stat);
	string eth1 = regFound(regEth1, stat);
	string lo = regFound(regLo, stat);
	netPrioMap["eth0"] = Util::atoi(eth0);
	netPrioMap["eth1"] = Util::atoi(eth1);
	netPrioMap["lo"] = Util::atoi(lo);
	return netPrioMap;
}

vector<int> Cgroup::getMemoryTasks(){
	string path = cgroupDirectory + "memory/tasks";
	syslog(LOG_DEBUG, "Reading from the file '%s'", path.c_str());
	string file = Util::readFile((cgroupDirectory + "memory/tasks").c_str());
	vector<int> tasks;
	size_t pos = 0;
	size_t ini = 0;
	while((pos = file.find('\n', ini)) != string::npos){
		tasks.push_back(Util::atoi(file.substr(ini, pos)));
		ini = pos + 1;
	}
	return tasks;
}

long int Cgroup::getMemoryLimitInBytes(){
	string path = cgroupDirectory + "memory/memory.limit_in_bytes";
	syslog(LOG_DEBUG, "Reading from the file '%s'", path.c_str());
	return Util::atol(Util::readFile((cgroupDirectory + "memory/memory.limit_in_bytes").c_str()));
}

map<string, long int> Cgroup::getMemoryStat(){
	map<string, long int> memStat;
	string stat;
	string path = cgroupDirectory + "memory/memory.stat";
	syslog(LOG_DEBUG, "Reading from the file '%s'", path.c_str());
	stat = Util::readFile((cgroupDirectory + "memory/memory.stat").c_str());

	string cache = regFound(regCache, stat);
	string shmem = regFound(regMem, stat);
	string mappedFile = regFound(regMapped, stat);
	string pgfault = regFound(regFault, stat);
	string hierarchicalLimit = regFound(regHierarchical, stat);

	memStat["cache"] = Util::atol(cache);
	memStat["shmem"] = Util::atol(shmem);
	memStat["mapped_file"] = Util::atol(mappedFile);
	memStat["pgfault"] = Util::atol(pgfault);
	memStat["hierarchical_memory_limit"] = Util::atol(hierarchicalLimit);
	return memStat;
}

long int Cgroup::getMemoryUsageInBytes(){
	string path = cgroupDirectory + "memory/memory.usage_in_bytes";
	syslog(LOG_DEBUG, "Reading from the file '%s'", path.c_str());
	return Util::atol(Util::readFile(cgroupDirectory + "memory/memory.usage_in_bytes"));
}

int Cgroup::getMemNotify(){
	string path = cgroupDirectory + "memory/notify_on_release";
	syslog(LOG_DEBUG, "Reading from the file '%s'", path.c_str());
	return Util::atoi(Util::readFile(cgroupDirectory + "memory/notify_on_release"));
}

string Cgroup::getMemReleaseAgent(){
	string path = cgroupDirectory + "memory/release_agent";
	syslog(LOG_DEBUG, "Reading from the file '%s'", path.c_str());
	return Util::readFile(cgroupDirectory + "memory/release_agent");
}

/**
 * Contains a flag that enables or disables the Out of Memory killer for a cgroup.
 */
map<string, int> Cgroup::getMemoryOOMControl(){
	string path = cgroupDirectory + "memory/memory.oom_control";
	syslog(LOG_DEBUG, "Reading from the file '%s'", path.c_str());
	string stat = Util::readFile((cgroupDirectory + "memory/memory.oom_control").c_str());
	map<string, int> memoryOOM;

	string oomDisable = regFound(regOOM, stat);
	string underOOM = regFound(regUnder, stat);
	string oomKill = regFound(regKill, stat);

	memoryOOM["oom_kill_disable"] = Util::atoi(oomDisable);
	memoryOOM["under_oom"] = Util::atoi(underOOM);
	memoryOOM["oom_kill"] = Util::atoi(oomKill);

	return memoryOOM;
}

/**
 *	Specify an internet interface with a number to set its priority
 *	1 being the top priority. E.g: eth0 2
 */
void Cgroup::setNetPrioMap(string interface){
	syslog(LOG_DEBUG,"Writing to the file '%s'", "net_prio/net_prio.ifpriomap");
	Util::writeFile(cgroupDirectory + "net_prio/net_prio.ifpriomap",interface);
	syslog(LOG_DEBUG,"'%s' has been successfully written", "net_prio/net_prio.ifpriomap");
}
/**
 * If the clone_children flag is enabled in a cgroup, a new cpuset cgroup
 * will copy its configuration from the parent during initialization
 */
void Cgroup::setCPUCloneChildren(bool flag){
	syslog(LOG_DEBUG,"Writing to the file '%s'", "cpu/cgroup.clone_children");
	Util::writeFile(cgroupDirectory + "cpu/cgroup.clone_children",flag?"1":"0");
	syslog(LOG_DEBUG,"'%s' has been successfully written", "cpu/cgroup.clone_children");
}
/**
 * Insert a process' PID to allow it to be in the CPU's controllers
 */
void Cgroup::setCPUProcs(int pid){
	syslog(LOG_DEBUG,"Writing to the file '%s'", "cpu/cgroup.procs");
	Util::writeFile(cgroupDirectory + "cpu/cgroup.procs",Util::itos(pid));
	syslog(LOG_DEBUG,"'%s' has been successfully written", "/pu/cgroup.procs");
}

/**
 * Contains a flag that indicates whether the cgroup will notify when
 * the CPU controller has no processes in it
 */
void Cgroup::setCPUNotify(bool flag){
	syslog(LOG_DEBUG,"Writing to the file '%s'", "cpu/notify_on_release");
	Util::writeFile(cgroupDirectory + "cpu/notify_on_release",flag?"1":"0");
	syslog(LOG_DEBUG,"'%s' has been successfully written", "cpu/notify_on_release");
}

/**
 * Specifies the path to the file in which the cgroup will notify
 * when it's empty. This requires the flag in notify_on_release to be set to 1
 */
void Cgroup::setCPUReleaseAgentPath(string path){
	syslog(LOG_DEBUG,"Writing to the file '%s'", "cpu/release_agent");
	Util::writeFile(cgroupDirectory + "cpu/release_agent",path);
	syslog(LOG_DEBUG,"'%s' has been successfully written", "cpu/release_agent");
}

/**
 * Specifies the CPUs that tasks in this cgroup are permitted to access.
 * This is a comma-separated list, with dashes to represent ranges.
 * For example: 0-2,16
 */
void Cgroup::setCPUs(string cpus){
	syslog(LOG_DEBUG,"Writing to the file '%s'", "cpuset/cpuset.cpus");
	Util::writeFile(cgroupDirectory + "cpuset/cpuset.cpus",cpus);
	syslog(LOG_DEBUG,"'%s' has been successfully written", "cpuset/cpuset.cpus");
}
