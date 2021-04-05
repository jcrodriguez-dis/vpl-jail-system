/**
 * package:		Part of vpl-jail-system
 * copyright:
 * license:		GNU/GPL, see LICENSE.txt or http://www.gnu.org/licenses/gpl-3.0.html
 **/

#include "cgroup.h"

using namespace std;

string Cgroup::baseCgroupFileSystem="/sys/fs/cgroup";
vplregex Cgroup::regUser("(^|\n)user ([0-9]+)(\n|$)");
vplregex Cgroup::regSystem("(^|\n)system ([0-9]+)(\n|$)");
vplregex Cgroup::regPeriods("(^|\n)nr_periods ([0-9]+)(\n|$)");
vplregex Cgroup::regThrottled("(^|\n)nr_throttled ([0-9]+)(\n|$)");
vplregex Cgroup::regThrottledTime("(^|\n)throttled_time ([0-9]+)(\n|$)");
vplregex Cgroup::regCache("(^|\n)cache ([0-9]+)(\n|$)");
vplregex Cgroup::regMem("(^|\n)shmem ([0-9]+)(\n|$)");
vplregex Cgroup::regMapped("(^|\n)mapped_file ([0-9]+)(\n|$)");
vplregex Cgroup::regFault("(^|\n)pgfault ([0-9]+)(\n|$)");
vplregex Cgroup::regHierarchical("(^|\n)hierarchical_memory_limit ([0-9]+)(\n|$)");
vplregex Cgroup::regEth0("(^|\n)eth0 ([0-9]+)(\n|$)");
vplregex Cgroup::regEth1("(^|\n)eth1 ([0-9]+)(\n|$)");
vplregex Cgroup::regLo("(^|\n)lo ([0-9]+)(\n|$)");
vplregex Cgroup::regOOM("(^|\n)oom_kill_disable ([0-9]+)(\n|$)");
vplregex Cgroup::regUnder("(^|\n)under_oom ([0-9]+)(\n|$)");
vplregex Cgroup::regKill("(^|\n)oom_kill ([0-9]+)(\n|$)");
vplregex Cgroup::regTrim("([ \n\t]*)([^ \n\t]+)([ \n\t]*)");

string Cgroup::regFound(vplregex &reg, string input){
	vplregmatch found;
	string match;
	bool matchFound = reg.search(input, found);
	if (matchFound) {
		match = found[2];
	}
	return match;
}

map<string, int> Cgroup::getCPUAcctStat(){
	map<string, int> cpuStat;
	string stat;
	syslog(LOG_DEBUG, "Reading from the file '%s'", (cgroupDirectory + "cpu,cpuacct/cpuacct.stat").c_str());
	stat = Util::readFile((cgroupDirectory + "cpu,cpuacct/cpuacct.stat").c_str());

	string sUser = regFound(regUser, stat);
	string sSystem = regFound(regSystem, stat);

	cpuStat["user"] = Util::atoi(sUser);
	cpuStat["system"] = Util::atoi(sSystem);
	return cpuStat;
}


long int Cgroup::getCPUUsage(){
	string path = cgroupDirectory + "cpu,cpuacct/cpuacct.usage";
	syslog(LOG_DEBUG, "Reading from the file '%s'", path.c_str());
	return Util::atol(Util::readFile((cgroupDirectory + "cpu,cpuacct/cpuacct.usage")));
}

int Cgroup::getCPUNotify(){
	string path = cgroupDirectory + "cpu,cpuacct/notify_on_release";
	syslog(LOG_DEBUG, "Reading from the file '%s'", path.c_str());
	return Util::atoi(Util::readFile((cgroupDirectory + "cpu,cpuacct/notify_on_release").c_str()));
}

string Cgroup::getCPUReleaseAgent(){
	string path = cgroupDirectory + "cpu,cpuacct/release_agent";
	syslog(LOG_DEBUG, "Reading from the file '%s'", path.c_str());
	string result = Util::readFile((cgroupDirectory + "cpu,cpuacct/release_agent").c_str());
	return regFound(regTrim, result);
}

vector<int> Cgroup::getCPUProcs(){
	string path = cgroupDirectory + "cpu,cpuacct/tasks";
	syslog(LOG_DEBUG, "Reading from the file '%s'", path.c_str());
	string procs = Util::readFile((cgroupDirectory + "cpu,cpuacct/tasks").c_str());
	vector<int> pids;
	size_t pos = 0;
	size_t ini = 0;
	while((pos = procs.find('\n', ini)) != string::npos){
		pids.push_back(Util::atoi(procs.substr(ini, pos)));
		ini = pos + 1;
	}
	return pids;
}

map<string, int> Cgroup::getCPUStat(){
	map<string, int> cpuStat;
	string path = cgroupDirectory + "cpu,cpuacct/cpu.stat";
	syslog(LOG_DEBUG, "Reading from the file '%s'", path.c_str());
	string stat = Util::readFile((cgroupDirectory + "cpu,cpuacct/cpu.stat").c_str());

	string nrPeriods = regFound(regPeriods, stat);
	string nrThrottled = regFound(regThrottled, stat);
	string throttledTime = regFound(regThrottledTime, stat);

	cpuStat["nr_periods"] = Util::atoi(nrPeriods);
	cpuStat["nr_throttled"] = Util::atoi(nrThrottled);
	cpuStat["throttled_time"] = Util::atoi(throttledTime);
	return cpuStat;
}

int Cgroup::getNetPrioID(){
	string path = cgroupDirectory + "net_cls,net_prio/net_prio.prioidx";
	syslog(LOG_DEBUG, "Reading from the file '%s'", path.c_str());
	return Util::atoi(Util::readFile((cgroupDirectory + "net_cls,net_prio/net_prio.prioidx").c_str()));
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
	string path = cgroupDirectory + "net_cls,net_prio/net_prio.ifpriomap";
	syslog(LOG_DEBUG, "Reading from the file '%s'", path.c_str());
	string stat = Util::readFile((cgroupDirectory + "net_cls,net_prio/net_prio.ifpriomap").c_str());
	string eth0 = regFound(regEth0, stat);
	string eth1 = regFound(regEth1, stat);
	string lo = regFound(regLo, stat);
	netPrioMap["eth0"] = Util::atoi(eth0);
	netPrioMap["eth1"] = Util::atoi(eth1);
	netPrioMap["lo"] = Util::atoi(lo);
	return netPrioMap;
}

int Cgroup::getNetNotify(){
	string path = cgroupDirectory + "net_cls,net_prio/notify_on_release";
	syslog(LOG_DEBUG, "Reading from the file '%s'", path.c_str());
	return Util::atoi(Util::readFile(cgroupDirectory + "net_cls,net_prio/notify_on_release"));
}

string Cgroup::getNetReleaseAgent(){
	string path = cgroupDirectory + "net_cls,net_prio/release_agent";
	syslog(LOG_DEBUG, "Reading from the file '%s'", path.c_str());
	return Util::readFile(cgroupDirectory + "net_cls,net_prio/release_agent");
}

vector<int> Cgroup::getNetProcs(){
	string path = cgroupDirectory + "net_cls,net_prio/tasks";
	syslog(LOG_DEBUG, "Reading from the file '%s'", path.c_str());
	string file = Util::readFile((cgroupDirectory + "net_cls,net_prio/tasks").c_str());
	vector<int> tasks;
	size_t pos = 0;
	size_t ini = 0;
	while((pos = file.find('\n', ini)) != string::npos){
		tasks.push_back(Util::atoi(file.substr(ini, pos)));
		ini = pos + 1;
	}
	return tasks;
}

vector<int> Cgroup::getMemoryProcs(){
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
	syslog(LOG_DEBUG,"Writing to the file '%s'", "net_cls,net_prio/net_prio.ifpriomap");
	Util::writeFile(cgroupDirectory + "net_cls,net_prio/net_prio.ifpriomap",interface);
	syslog(LOG_DEBUG,"'%s' has been successfully written", "net_cls,net_prio/net_prio.ifpriomap");
}

void Cgroup::setNetNotify(bool flag){
	syslog(LOG_DEBUG,"Writing to the file '%s'", "net_cls,net_prio/notify_on_release");
	Util::writeFile(cgroupDirectory + "net_cls,net_prio/notify_on_release",flag?"1":"0");
	syslog(LOG_DEBUG,"'%s' has been successfully written", "net_cls,net_prio/notify_on_release");
}

void Cgroup::setNetReleaseAgent(string path){
	syslog(LOG_DEBUG,"Writing to the file '%s'", "net_cls,net_prio/release_agent");
	Util::writeFile(cgroupDirectory + "net_cls,net_prio/release_agent",path);
	syslog(LOG_DEBUG,"'%s' has been successfully written", "net_cls,net_prio/release_agent");
}

void Cgroup::setNetProcs(int pid){
	syslog(LOG_DEBUG,"Writing to the file '%s'", "net_cls,net_prio/tasks");
	ofstream file;
	file.open(cgroupDirectory + "net_cls,net_prio/tasks", fstream::app);
	if (file.is_open()){
		file << Util::itos(pid) + '\n';
		syslog(LOG_DEBUG,"'%s' has been successfully written", "net_cls,net_prio/tasks");
	}
	file.close();
}
/**
 * Insert a process' PID to allow it to be in the CPU controllers
 */
void Cgroup::setCPUProcs(int pid){
	syslog(LOG_DEBUG,"Writing to the file '%s'", "cpu,cpuacct/tasks");
	ofstream file;
	file.open(cgroupDirectory + "cpu,cpuacct/tasks", fstream::app);
	if (file.is_open()){
		file << Util::itos(pid) + '\n';
		syslog(LOG_DEBUG,"'%s' has been successfully written", "cpu,cpuacct/tasks");
	}
	file.close();
}

/**
 * Contains a flag that indicates whether the cgroup will notify when
 * the CPU controller has no processes in it
 */
void Cgroup::setCPUNotify(bool flag){
	syslog(LOG_DEBUG,"Writing to the file '%s'", "cpu,cpuacct/notify_on_release");
	Util::writeFile(cgroupDirectory + "cpu,cpuacct/notify_on_release",flag?"1":"0");
	syslog(LOG_DEBUG,"'%s' has been successfully written", "cpu,cpuacct/notify_on_release");
}

/**
 * Specifies the path to the file in which the cgroup will notify
 * when the cpu tasks file is empty. This requires the flag in notify_on_release to be set to 1
 */
void Cgroup::setCPUReleaseAgentPath(string path){
	syslog(LOG_DEBUG,"Writing to the file '%s'", "cpu,cpuacct/release_agent");
	Util::writeFile(cgroupDirectory + "cpu,cpuacct/release_agent",path);
	syslog(LOG_DEBUG,"'%s' has been successfully written", "cpu,cpuacct/release_agent");
}

/**
 * Insert a process' PID to allow it to be in the memory controllers
 */
void Cgroup::setMemoryProcs(int pid){
	syslog(LOG_DEBUG,"Writing to the file '%s'", "memory/tasks");
	ofstream file;
	file.open(cgroupDirectory + "memory/tasks", fstream::app);
	if (file.is_open()){
		file << Util::itos(pid) + '\n';
		syslog(LOG_DEBUG,"'%s' has been successfully written", "memory/tasks");
	}
	file.close();
}

/**
 * Set a limit in bytes for the memory controller
 */
void Cgroup::setMemoryLimitInBytes(long int bytes){
	syslog(LOG_DEBUG,"Writing to the file '%s'", "memory/memory.limit_in_bytes");
	Util::writeFile(cgroupDirectory + "memory/memory.limit_in_bytes",Util::itos(bytes));
	syslog(LOG_DEBUG,"'%s' has been successfully written", "memory/memory.limit_in_bytes");
}

/**
 * Contains a flag that indicates whether the cgroup will notify when
 * the memory controller has no processes in it
 */
void Cgroup::setMemNotify(bool flag){
	syslog(LOG_DEBUG,"Writing to the file '%s'", "memory/notify_on_release");
	Util::writeFile(cgroupDirectory + "memory/notify_on_release",flag?"1":"0");
	syslog(LOG_DEBUG,"'%s' has been successfully written", "memory/notify_on_release");
}

/**
 * Specifies the path to the file in which the cgroup will notify
 * when the memory tasks file is empty. This requires the flag in notify_on_release to be set to 1
 */
void Cgroup::setMemReleaseAgentPath(string path){
	syslog(LOG_DEBUG,"Writing to the file '%s'", "memory/release_agent");
	Util::writeFile(cgroupDirectory + "memory/release_agent",path);
	syslog(LOG_DEBUG,"'%s' has been successfully written", "memory/release_agent");
}
