/**
 * package:		Part of vpl-jail-system
 * copyright:
 * license:		GNU/GPL, see LICENSE.txt or http://www.gnu.org/licenses/gpl-3.0.html
 **/

#include "cgroup.h"
#include <regex.h>

using namespace std;
string Cgroup::baseCgroupFileSystem="/sys/fs/cgroup";



string Cgroup::regFound(regex_t reg, string input){
	regmatch_t found[1];
	string match;
	int matchFound = regexec(&reg,input.c_str(),1,found,0);
	if (matchFound != 0) {
		match = "";
	} else {
		match = input.substr(found[0].rm_so, found[0].rm_eo);
	}
	return match;
}

map<string, int> Cgroup::getCPUAcctStat(){
	map<string, int> cpuStat;
	string stat;
	syslog(LOG_DEBUG, "Reading from the file '%s'", (cgroupDirectory + "cpu/cpuacct.stat").c_str());
	stat = Util::readFile((cgroupDirectory + "cpu/cpuacct.stat").c_str());

	string sUser = regFound(regUser, stat);
	sUser = sUser.substr(sUser.find(" "),sUser.length());
	string sSystem = regFound(regSystem, stat);
	sSystem = sSystem.substr(sSystem.find(" "),sSystem.length());

	cpuStat["user"] = Util::atoi(sUser);
	cpuStat["system"] = Util::atoi(sSystem);
	return cpuStat;
}


int Cgroup::getCPUUsage(){
	string path = cgroupDirectory + "cpu/cpuacct.usage";
	syslog(LOG_DEBUG, "Reading from the file '%s'", path.c_str());
	return Util::atoi(Util::readFile((cgroupDirectory + "cpu/cpuacct.usage").c_str()));
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
	nrPeriods = nrPeriods.substr(nrPeriods.find(" "),nrPeriods.length());
	string nrThrottled = regFound(regThrottled, stat);
	nrThrottled = nrThrottled.substr(nrThrottled.find(" "),nrThrottled.length());
	string throttledTime = regFound(regThrottledTime, stat);
	throttledTime = throttledTime.substr(throttledTime.find(" "),throttledTime.length());

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
	while((pos = file.find(ini, '\n')) != string::npos){
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

string Cgroup::getMemoryTasks(){
	string path = cgroupDirectory + "memory/tasks";
	syslog(LOG_DEBUG, "Reading from the file '%s'", path.c_str());
	return Util::readFile((cgroupDirectory + "memory/tasks").c_str());
}

long int Cgroup::getMemoryLimitInBytes(){
	string path = cgroupDirectory + "memory/memory.limit_in_bytes";
	syslog(LOG_DEBUG, "Reading from the file '%s'", path.c_str());
	return Util::atol(Util::readFile((cgroupDirectory + "memory/memory.limit_in_bytes").c_str()));
}

map<string, int> Cgroup::getMemoryStat(){
	map<string, int> memStat;
	string stat;
	string path = cgroupDirectory + "memory/memory.stat";
	syslog(LOG_DEBUG, "Reading from the file '%s'", path.c_str());
	stat = Util::readFile((cgroupDirectory + "memory/memory.stat").c_str());

	string cache = regFound(regCache, stat);
	cache = cache.substr(cache.find(" "),cache.length());

	string shmem = regFound(regMem, stat);
	shmem = shmem.substr(shmem.find(" "),shmem.length());
	syslog(LOG_DEBUG, "shmem = '%s'", shmem.c_str());

	string mappedFile = regFound(regMapped, stat);
	mappedFile = mappedFile.substr(mappedFile.find(" "),mappedFile.length());
	syslog(LOG_DEBUG, "mappedFile = '%s'", mappedFile.c_str());

	string pgfault = regFound(regFault, stat);
	pgfault = pgfault.substr(pgfault.find(" "),pgfault.length());
	syslog(LOG_DEBUG, "pgfault = '%s'", pgfault.c_str());

	string hierarchicalLimit = regFound(regHierarchical, stat);
	hierarchicalLimit = hierarchicalLimit.substr(hierarchicalLimit.find(" "),hierarchicalLimit.length());

	memStat["cache"] = Util::atoi(cache);
	memStat["shmem"] = Util::atoi(shmem);
	memStat["mapped_file"] = Util::atoi(mappedFile);
	memStat["pgfault"] = Util::atoi(pgfault);
	memStat["hierarchical_memory_limit"] = Util::atoi(hierarchicalLimit);
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
int Cgroup::getMemoryOOMControl(){
	string path = cgroupDirectory + "memory/memory.oom_control";
	syslog(LOG_DEBUG, "Reading from the file '%s'", path.c_str());
	return Util::atoi(Util::readFile(cgroupDirectory + "memory/memory.oom_control"));
}

/**
 * If the clone_children flag is enabled in a cgroup, a new cpuset cgroup
 * will copy its configuration from the parent during initialization
 */
void Cgroup::setCPUCloneChildren(bool flag){
	syslog(LOG_DEBUG,"Writing to the file '%s'", "/cpu/cgroup.clone_children");
	Util::writeFile(cgroupDirectory + "cpu/cgroup.clone_children",flag?"1":"0");
	syslog(LOG_DEBUG,"'%s' has been successfully written", "/cpu/cgroup.clone_children");
}

/**
 * Insert a process' PID to allow it to be in the CPU's controllers
 */
void Cgroup::setCPUProcs(int pid){
	syslog(LOG_DEBUG,"Writing to the file '%s'", "/cpu/cgroup.procs");
	Util::writeFile(cgroupDirectory + "cpu/cgroup.procs",Util::itos(pid));
	syslog(LOG_DEBUG,"'%s' has been successfully written", "/cpu/cgroup.procs");
}

/**
 * Contains an integer value that specifies a relative share of CPU time
 * available to the tasks in a cgroup. The value specified must be 2 or higher
 * A cgroup with a value set to 200 will receive twice the CPU time of tasks
 * in a cgroup with value set to 100.
 */
void Cgroup::setCPUShares(int share){
	syslog(LOG_DEBUG,"Writing to the file '%s'", "/cpu/cpu.shares");
	Util::writeFile(cgroupDirectory + "cpu/cpu.shares",Util::itos(share));
	syslog(LOG_DEBUG,"'%s' has been successfully written", "/cpu/cpu.shares");
}

void Cgroup::setCPUNotify(bool flag){
	syslog(LOG_DEBUG,"Writing to the file '%s'", "/cpu/notify_on_release");
	Util::writeFile(cgroupDirectory + "cpu/notify_on_release",flag?"1":"0");
	syslog(LOG_DEBUG,"'%s' has been successfully written", "/cpu/notify_on_release");
}

void Cgroup::setCPUReleaseAgentPath(string path){
	syslog(LOG_DEBUG,"Writing to the file '%s'", "/cpu/release_agent");
	Util::writeFile(cgroupDirectory + "cpu/release_agent",path);
	syslog(LOG_DEBUG,"'%s' has been successfully written", "/cpu/release_agent");
}

/**
 * Specifies the CPUs that tasks in this cgroup are permitted to access.
 * This is a comma-separated list, with dashes to represent ranges.
 * For example: 0-2,16
 */
void Cgroup::setCPUs(string cpus){
	syslog(LOG_DEBUG,"Writing to the file '%s'", "/cpuset/cpuset.cpus");
	Util::writeFile(cgroupDirectory + "cpuset/cpuset.cpus",cpus);
	syslog(LOG_DEBUG,"'%s' has been successfully written", "/cpuset/cpuset.cpus");
}

/**
 * Contains a flag that specifies whether cpusets other than this one and its
 * parents and children can share the memory nodes specified for this cpuset. By default(0),
 * memory nodes are not allocated exclusively to one cpuset
 */
void Cgroup::setMemExclusive(bool flag){
	syslog(LOG_DEBUG,"Writing to the file '%s'", "cpu/cpuset.mem_exclusive");
	Util::writeFile(cgroupDirectory + "cpu/cpuset.mem_exclusive",flag?"1":"0");
	syslog(LOG_DEBUG,"'%s' has been successfully written", "cpu/cpuset.mem_exclusive");
}


/**
 * Contains a flag that specifies whether the system should compute the memory
 * pressure. Computed values are output to cpuset.emmory_pressure and represent
 * the rate at which processes attempt to free in-use memory, reported as an
 * integer value of attempts to reclaim memory per second, multiplied by 1000.
 */
void Cgroup::setMemoryPressure(bool flag){
	syslog(LOG_DEBUG,"Writing to the file '%s'", "/cpu/cpuset.memory_pressure_enabled");
	Util::writeFile(cgroupDirectory + "cpu/cpuset.memory_pressure_enabled",flag?"1":"0");
	syslog(LOG_DEBUG,"'%s' has been successfully written", "/cpu/cpuset.memory_pressure_enabled");
}

/**
 * Contains a flag that specifies whether file system buffers should be spread
 * evenly across the memory nodes allocated to the cpuset.
 */
void Cgroup::setMemorySpreadPage(bool flag){
	syslog(LOG_DEBUG,"Writing to the file '%s'", "/cpu/cpuset.memory_spread_page");
	Util::writeFile(cgroupDirectory + "cpu/cpuset.memory_spread_page",flag?"1":"0");
	syslog(LOG_DEBUG,"'%s' has been successfully written", "/cpu/cpuset.memory_spread_page");
}
