/**
 * package:		Part of vpl-jail-system
 * copyright:
 * license:		GNU/GPL, see LICENSE.txt or http://www.gnu.org/licenses/gpl-3.0.html
 **/

#include "cgroup.h"
#include "util.h"

using namespace std;

string Cgroup::baseCgroupFileSystem= Util::detectCgroupPath();
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

const char* Cgroup::FILE_CPU_ACCT_STAT = "cpu,cpuacct/cpuacct.stat";
const char* Cgroup::FILE_CPU_USAGE = "cpu,cpuacct/cpuacct.usage";
const char* Cgroup::FILE_CPU_NOTIFY = "cpu,cpuacct/notify_on_release";
const char* Cgroup::FILE_CPU_RELEASE_AGENT = "cpu,cpuacct/release_agent";
const char* Cgroup::FILE_CPU_TASKS = "cpu,cpuacct/tasks";
const char* Cgroup::FILE_CPU_STAT = "cpu,cpuacct/cpu.stat";

const char* Cgroup::FILE_NET_PRIOIDX = "net_cls,net_prio/net_prio.prioidx";
const char* Cgroup::FILE_NET_IFPRIOMAP = "net_cls,net_prio/net_prio.ifpriomap";
const char* Cgroup::FILE_NET_NOTIFY = "net_cls,net_prio/notify_on_release";
const char* Cgroup::FILE_NET_RELEASE_AGENT = "net_cls,net_prio/release_agent";
const char* Cgroup::FILE_NET_TASKS = "net_cls,net_prio/tasks";

const char* Cgroup::FILE_PIDS_PROCS = "pids/cgroup.procs";

const char* Cgroup::FILE_MEM_TASKS = "memory/tasks";
const char* Cgroup::FILE_MEM_LIMIT = "memory/memory.limit_in_bytes";
const char* Cgroup::FILE_MEM_STAT = "memory/memory.stat";
const char* Cgroup::FILE_MEM_USAGE = "memory/memory.usage_in_bytes";
const char* Cgroup::FILE_MEM_NOTIFY = "memory/notify_on_release";
const char* Cgroup::FILE_MEM_RELEASE_AGENT = "memory/release_agent";
const char* Cgroup::FILE_MEM_OOM_CONTROL = "memory/memory.oom_control";

string Cgroup::regFound(vplregex &reg, string input){
	vplregmatch found(4);
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
    string path = cgroupDirectory + FILE_CPU_ACCT_STAT;
    Logger::log(LOG_DEBUG, "Reading from the file '%s'", path.c_str());
    stat = Util::readFile(path);

    string sUser = regFound(regUser, stat);
    string sSystem = regFound(regSystem, stat);

    cpuStat["user"] = Util::atoi(sUser);
    cpuStat["system"] = Util::atoi(sSystem);
    return cpuStat;
}


long int Cgroup::getCPUUsage(){
    string path = cgroupDirectory + FILE_CPU_USAGE;
    Logger::log(LOG_DEBUG, "Reading from the file '%s'", path.c_str());
    return Util::atol(Util::readFile(path));
}

int Cgroup::getCPUNotify(){
    string path = cgroupDirectory + FILE_CPU_NOTIFY;
    Logger::log(LOG_DEBUG, "Reading from the file '%s'", path.c_str());
    return Util::atoi(Util::readFile(path));
}

string Cgroup::getCPUReleaseAgent(){
    string path = cgroupDirectory + FILE_CPU_RELEASE_AGENT;
    Logger::log(LOG_DEBUG, "Reading from the file '%s'", path.c_str());
    string result = Util::readFile(path);
    return regFound(regTrim, result);
}

vector<int> Cgroup::getCPUProcs(){
    string path = cgroupDirectory + FILE_CPU_TASKS;
    Logger::log(LOG_DEBUG, "Reading from the file '%s'", path.c_str());
    string procs = Util::readFile(path);
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
    string path = cgroupDirectory + FILE_CPU_STAT;
    Logger::log(LOG_DEBUG, "Reading from the file '%s'", path.c_str());
    string stat = Util::readFile(path);

    string nrPeriods = regFound(regPeriods, stat);
    string nrThrottled = regFound(regThrottled, stat);
    string throttledTime = regFound(regThrottledTime, stat);

    cpuStat["nr_periods"] = Util::atoi(nrPeriods);
    cpuStat["nr_throttled"] = Util::atoi(nrThrottled);
    cpuStat["throttled_time"] = Util::atoi(throttledTime);
    return cpuStat;
}

int Cgroup::getNetPrioID(){
    string path = cgroupDirectory + FILE_NET_PRIOIDX;
    Logger::log(LOG_DEBUG, "Reading from the file '%s'", path.c_str());
    return Util::atoi(Util::readFile(path));
}

vector<int> Cgroup::getPIDs(){
    string path = cgroupDirectory + FILE_PIDS_PROCS;
    Logger::log(LOG_DEBUG, "Reading from the file '%s'", path.c_str());
    string file = Util::readFile(path);
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
    string path = cgroupDirectory + FILE_NET_IFPRIOMAP;
    Logger::log(LOG_DEBUG, "Reading from the file '%s'", path.c_str());
    string stat = Util::readFile(path);
    string eth0 = regFound(regEth0, stat);
    string eth1 = regFound(regEth1, stat);
    string lo = regFound(regLo, stat);
    netPrioMap["eth0"] = Util::atoi(eth0);
    netPrioMap["eth1"] = Util::atoi(eth1);
    netPrioMap["lo"] = Util::atoi(lo);
    return netPrioMap;
}

int Cgroup::getNetNotify(){
    string path = cgroupDirectory + FILE_NET_NOTIFY;
    Logger::log(LOG_DEBUG, "Reading from the file '%s'", path.c_str());
    return Util::atoi(Util::readFile(path));
}

string Cgroup::getNetReleaseAgent(){
    string path = cgroupDirectory + FILE_NET_RELEASE_AGENT;
    Logger::log(LOG_DEBUG, "Reading from the file '%s'", path.c_str());
    return Util::readFile(path);
}

vector<int> Cgroup::getNetProcs(){
    string path = cgroupDirectory + FILE_NET_TASKS;
    Logger::log(LOG_DEBUG, "Reading from the file '%s'", path.c_str());
    string file = Util::readFile(path);
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
    string path = cgroupDirectory + FILE_MEM_TASKS;
    Logger::log(LOG_DEBUG, "Reading from the file '%s'", path.c_str());
    string file = Util::readFile(path);
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
    string path = cgroupDirectory + FILE_MEM_LIMIT;
    Logger::log(LOG_DEBUG, "Reading from the file '%s'", path.c_str());
    return Util::atol(Util::readFile(path));
}

map<string, long int> Cgroup::getMemoryStat(){
    map<string, long int> memStat;
    string stat;
    string path = cgroupDirectory + FILE_MEM_STAT;
    Logger::log(LOG_DEBUG, "Reading from the file '%s'", path.c_str());
    stat = Util::readFile(path);

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
    string path = cgroupDirectory + FILE_MEM_USAGE;
    Logger::log(LOG_DEBUG, "Reading from the file '%s'", path.c_str());
    return Util::atol(Util::readFile(path));
}

int Cgroup::getMemNotify(){
    string path = cgroupDirectory + FILE_MEM_NOTIFY;
    Logger::log(LOG_DEBUG, "Reading from the file '%s'", path.c_str());
    return Util::atoi(Util::readFile(path));
}

string Cgroup::getMemReleaseAgent(){
    string path = cgroupDirectory + FILE_MEM_RELEASE_AGENT;
    Logger::log(LOG_DEBUG, "Reading from the file '%s'", path.c_str());
    return Util::readFile(path);
}

map<string, int> Cgroup::getMemoryOOMControl(){
    string path = cgroupDirectory + FILE_MEM_OOM_CONTROL;
    Logger::log(LOG_DEBUG, "Reading from the file '%s'", path.c_str());
    string stat = Util::readFile(path);
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
    string path = cgroupDirectory + FILE_NET_IFPRIOMAP;
    Logger::log(LOG_DEBUG,"Writing to the file '%s'", path.c_str());
    Util::writeFile(path, interface);
    Logger::log(LOG_DEBUG,"'%s' has been successfully written", path.c_str());
}

void Cgroup::setNetNotify(bool flag){
    string path = cgroupDirectory + FILE_NET_NOTIFY;
    Logger::log(LOG_DEBUG,"Writing to the file '%s'", path.c_str());
    Util::writeFile(path, flag?"1":"0");
    Logger::log(LOG_DEBUG,"'%s' has been successfully written", path.c_str());
}

void Cgroup::setNetReleaseAgent(string agentPath){
    string path = cgroupDirectory + FILE_NET_RELEASE_AGENT;
    Logger::log(LOG_DEBUG,"Writing to the file '%s'", path.c_str());
    Util::writeFile(path, agentPath);
    Logger::log(LOG_DEBUG,"'%s' has been successfully written", path.c_str());
}

void Cgroup::setNetProcs(int pid){
    string path = cgroupDirectory + FILE_NET_TASKS;
    Logger::log(LOG_DEBUG,"Writing to the file '%s'", path.c_str());
    ofstream file;
    file.open(path, fstream::app);
    if (file.is_open()){
        file << Util::itos(pid) + '\n';
        Logger::log(LOG_DEBUG,"'%s' has been successfully written", path.c_str());
    }
    file.close();
}
/**
 * Insert a process' PID to allow it to be in the CPU controllers
 */
void Cgroup::setCPUProcs(int pid){
    string path = cgroupDirectory + FILE_CPU_TASKS;
    Logger::log(LOG_DEBUG,"Writing to the file '%s'", path.c_str());
    ofstream file;
    file.open(path, fstream::app);
    if (file.is_open()){
        file << Util::itos(pid) + '\n';
        Logger::log(LOG_DEBUG,"'%s' has been successfully written", path.c_str());
        file.close();
    } else {
        Logger::log(LOG_ERR, "Failed to open file '%s' for writing", path.c_str());
    }
}

/**
 * Contains a flag that indicates whether the cgroup will notify when
 * the CPU controller has no processes in it
 */
void Cgroup::setCPUNotify(bool flag){
    string path = cgroupDirectory + FILE_CPU_NOTIFY;
    Logger::log(LOG_DEBUG,"Writing to the file '%s'", path.c_str());
    Util::writeFile(path, flag?"1":"0");
    Logger::log(LOG_DEBUG,"'%s' has been successfully written", path.c_str());
}

/**
 * Specifies the path to the file in which the cgroup will notify
 * when the cpu tasks file is empty. This requires the flag in notify_on_release to be set to 1
 */
void Cgroup::setCPUReleaseAgentPath(string agentPath){
    string path = cgroupDirectory + FILE_CPU_RELEASE_AGENT;
    Logger::log(LOG_DEBUG,"Writing to the file '%s'", path.c_str());
    Util::writeFile(path, agentPath);
    Logger::log(LOG_DEBUG,"'%s' has been successfully written", path.c_str());
}

/**
 * Insert a process' PID to allow it to be in the memory controllers
 */
void Cgroup::setMemoryProcs(int pid){
    string path = cgroupDirectory + FILE_MEM_TASKS;
    Logger::log(LOG_DEBUG,"Writing to the file '%s'", path.c_str());
    ofstream file;
    file.open(path, fstream::app);
    if (file.is_open()){
        file << Util::itos(pid) + '\n';
        Logger::log(LOG_DEBUG,"'%s' has been successfully written", path.c_str());
    }
    file.close();
}

/**
 * Set a limit in bytes for the memory controller
 */
void Cgroup::setMemoryLimitInBytes(long int bytes){
    string path = cgroupDirectory + FILE_MEM_LIMIT;
    Logger::log(LOG_DEBUG,"Writing to the file '%s'", path.c_str());
    Util::writeFile(path, Util::itos(bytes));
    Logger::log(LOG_DEBUG,"'%s' has been successfully written", path.c_str());
}

/**
 * Contains a flag that indicates whether the cgroup will notify when
 * the memory controller has no processes in it
 */
void Cgroup::setMemNotify(bool flag){
    string path = cgroupDirectory + FILE_MEM_NOTIFY;
    Logger::log(LOG_DEBUG,"Writing to the file '%s'", path.c_str());
    Util::writeFile(path, flag?"1":"0");
    Logger::log(LOG_DEBUG,"'%s' has been successfully written", path.c_str());
}

/**
 * Specifies the path to the file in which the cgroup will notify
 * when the memory tasks file is empty. This requires the flag in notify_on_release to be set to 1
 */
void Cgroup::setMemReleaseAgentPath(string agentPath){
    string path = cgroupDirectory + FILE_MEM_RELEASE_AGENT;
    Logger::log(LOG_DEBUG,"Writing to the file '%s'", path.c_str());
    Util::writeFile(path, agentPath);
    Logger::log(LOG_DEBUG,"'%s' has been successfully written", path.c_str());
}
