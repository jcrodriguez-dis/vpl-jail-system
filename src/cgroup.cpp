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
	string sSytem = regFound(regSystem, stat);
	// Comprobar regexec
	// Comprobar resultado regcomp sea 0
	cpuStat["user"] = Util::atoi(sUser);
	cpuStat["system"] = Util::atoi(sSytem);
	return cpuStat;
}

int Cgroup::getCPUCfsPeriod(){
	string directory = cgroupDirectory + "cpu/cpu.cfs_period_us";
	syslog(LOG_DEBUG, "Reading from the file '%s'", directory.c_str());
	return Util::atoi(Util::readFile((cgroupDirectory + "cpu/cpu.cfs_period_us").c_str()));
}

int Cgroup::getCPUCfsQuota(){
	string directory = cgroupDirectory + "cpu/cpu.cfs_quota_us";
	syslog(LOG_DEBUG, "Reading from the file '%s'", directory.c_str());
	return Util::atoi(Util::readFile((cgroupDirectory + "cpu/cpu.cfs_quota_us").c_str()));
}

long int Cgroup::getCPUUsage(){
	string directory = cgroupDirectory + "cpu/cpuacct.usage";
	syslog(LOG_DEBUG, "Reading from the file '%s'", directory.c_str());
	return Util::atol(Util::readFile((cgroupDirectory + "cpu/cpuacct.usage").c_str()));
}

int Cgroup::getNotify(){
	string directory = cgroupDirectory + "cpu/notify_on_release";
	syslog(LOG_DEBUG, "Reading from the file '%s'", directory.c_str());
	return Util::atoi(Util::readFile((cgroupDirectory + "cpu/notify_on_release").c_str()));
}

string Cgroup::getReleaseAgent(){
	string directory = cgroupDirectory + "cpu/release_agent";
	syslog(LOG_DEBUG, "Reading from the file '%s'", directory.c_str());
	return Util::readFile((cgroupDirectory + "cpu/release_agent").c_str());
}

string Cgroup::getCPUTasks(){
	string directory = cgroupDirectory + "cpu/tasks";
	syslog(LOG_DEBUG, "Reading from the file '%s'", directory.c_str());
	return Util::readFile((cgroupDirectory + "cpu/tasks").c_str());
}

map<string, int> Cgroup::getCPUStat(){
	map<string, int> cpuStat;
	string directory = cgroupDirectory + "cpu/cpu.stat";
	syslog(LOG_DEBUG, "Reading from the file '%s'", directory.c_str());
	string stat = Util::readFile((cgroupDirectory + "cpu/cpu.stat").c_str());
	string nrPeriods = regFound(regPeriods, stat);
	string nrThrottled = regFound(regThrottled, stat);
	string throttledTime = regFound(regThrottledTime, stat);
	cpuStat["nr_periods"] = Util::atoi(nrPeriods);
	cpuStat["nr_throttled"] = Util::atoi(nrThrottled);
	cpuStat["throttled_time"] = Util::atoi(throttledTime);
	return cpuStat;
}

int Cgroup::getCloneChildren(){
	string directory = cgroupDirectory + "cpu/cgroup.clone_children";
	syslog(LOG_DEBUG, "Reading from the file '%s'", directory.c_str());
	return Util::atoi(Util::readFile((cgroupDirectory + "cpu/cgroup.clone_children").c_str()));
}

string Cgroup::getMemoryTasks(){
	string directory = cgroupDirectory + "memory/tasks";
	syslog(LOG_DEBUG, "Reading from the file '%s'", directory.c_str());
	return Util::readFile((cgroupDirectory + "memory/tasks").c_str());
}

int Cgroup::getMemCloneChildren(){
	string directory = cgroupDirectory + "cpu/cgroup.clone_children";
	syslog(LOG_DEBUG, "Reading from the file '%s'", directory.c_str());
	return Util::atoi(Util::readFile((cgroupDirectory + "cpu/cgroup.clone_children").c_str()));
}

bool Cgroup::getMemoryMigrate(){
	string directory = cgroupDirectory + "cpuset/cpuset.memory_migrate";
	syslog(LOG_DEBUG, "Reading from the file '%s'", directory.c_str());
	if (Util::readFile(cgroupDirectory + "cpuset/cpuset.memory_migrate") == "1"){
		return true;
	} else {
		return false;
	}
}

long int Cgroup::getMemoryLimitInBytes(){
	string directory = cgroupDirectory + "memory/memory.limit_in_bytes";
	syslog(LOG_DEBUG, "Reading from the file '%s'", directory.c_str());
	return Util::atol(Util::readFile((cgroupDirectory + "memory/memory.limit_in_bytes").c_str()));
}

map<string, int> Cgroup::getMemoryStat(){
	map<string, int> memStat;
	string stat;
	string directory = cgroupDirectory + "memory/memory.stat";
	syslog(LOG_DEBUG, "Reading from the file '%s'", directory.c_str());
	stat = Util::readFile((cgroupDirectory + "memory/memory.stat").c_str());

	string cache = regFound(regCache, stat);
	string shmem = regFound(regMem, stat);
	string mappedFile = regFound(regMapped, stat);
	string pgfault = regFound(regFault, stat);
	string hierarchicalLimit = regFound(regHierarchical, stat);
	memStat["cache"] = Util::atoi(cache);
	memStat["shmem"] = Util::atoi(shmem);
	memStat["mapped_file"] = Util::atoi(mappedFile);
	memStat["pgfault"] = Util::atoi(pgfault);
	memStat["hierarchical_memory_limit"] = Util::atoi(hierarchicalLimit);
	return memStat;
}

/**
 * Shows the tendency of the kernel to swap out process memory used by tasks
 * The default value is 60. Values lower than 60 decrease the kernel's tendency to swap out memory
 * Values greater than 60 increase the kernel's tendency to swap out memory and values
 * greater than 100 permit the kernel to swap out pages that are part of the address
 * space of the processes in this cgroup
 */

int Cgroup::getMemSwappiness(){
	string directory = cgroupDirectory + "memory/memory.swappiness";
	syslog(LOG_DEBUG, "Reading from the file '%s'", directory.c_str());
	return Util::atoi(Util::readFile(cgroupDirectory + "memory/memory.swappiness"));
}

long int Cgroup::getMemoryUsageInBytes(){
	string directory = cgroupDirectory + "memory/memory.usage_in_bytes";
	syslog(LOG_DEBUG, "Reading from the file '%s'", directory.c_str());
	return Util::atol(Util::readFile(cgroupDirectory + "memory/memory.usage_in_bytes"));
}

int Cgroup::getMemNotify(){
	string directory = cgroupDirectory + "memory/notify_on_release";
	syslog(LOG_DEBUG, "Reading from the file '%s'", directory.c_str());
	return Util::atoi(Util::readFile(cgroupDirectory + "memory/notify_on_release"));
}

bool Cgroup::setMemHardwall(){
	string directory = cgroupDirectory + "cpu/cpuset.mem_hardwall";
	syslog(LOG_DEBUG,"Reading from the file '%s'", directory.c_str());
	if (Util::readFile(cgroupDirectory + "cpu/cpuset.mem_hardwall") == "1"){
		return true;
	} else {
		return false;
	}
}

string Cgroup::getMemReleaseAgent(){
	string directory = cgroupDirectory + "memory/release_agent";
	syslog(LOG_DEBUG, "Reading from the file '%s'", directory.c_str());
	return Util::readFile(cgroupDirectory + "memory/release_agent");
}

long int Cgroup::getMemoryFailCnt(){
	string directory = cgroupDirectory + "memory/memory.usage_in_bytes";
	syslog(LOG_DEBUG, "Reading from the file '%s'", directory.c_str());
	return Util::atol(Util::readFile(cgroupDirectory + "memory/memory.failcnt"));
}

/**
 * Allows moving charges associated with a task along with task migration. Charging is a
 * way of giving a penalty to cgroups which access shared pages too often.
 */

int Cgroup::getMemoryMoveChargeImmigrate(){
	string directory = cgroupDirectory + "memory/memory.move_charge_at_immigrate";
	syslog(LOG_DEBUG, "Reading from the file '%s'", directory.c_str());
	return Util::atoi(Util::readFile(cgroupDirectory + "memory/memory.move_charge_at_immigrate"));
}

/**
 * Contains a flag that specifies whether memory usage should be accounted for throughout
 * a hierarchy of cgroups.
 */
int Cgroup::getMemoryUseHierarchy(){
	string directory = cgroupDirectory + "memory/memory.use_hierarchy";
	syslog(LOG_DEBUG, "Reading from the file '%s'", directory.c_str());
	return Util::atoi(Util::readFile(cgroupDirectory + "memory/memory.use_hierarchy"));
}

/**
 * Contains a flag that enables or disables the Out of Memory killer for a cgroup.
 */
int Cgroup::getMemoryOOMControl(){
	string directory = cgroupDirectory + "memory/memory.oom_control";
	syslog(LOG_DEBUG, "Reading from the file '%s'", directory.c_str());
	return Util::atoi(Util::readFile(cgroupDirectory + "memory/memory.oom_control"));
}

/**
 * Specifies a period of time in microseconds for how regularly a cgroup's
 * access to CPU resources should be reallocated
 */

void Cgroup::setCPUCfsPeriod(int period){
	syslog(LOG_DEBUG,"Writing to the file '%s'", "/cpu/cpu.cfs_period_us");
	Util::writeFile(cgroupDirectory + "cpu/cpu.cfs_period_us",Util::itos(period));
	syslog(LOG_DEBUG,"'%s' has been successfully written", "/cpu/cpu.cfs_period_us");
}

/**
 * Specifies the total amount of time in microseconds for which all tasks in a
 * cgroup can run during one period
 */
void Cgroup::setCPUCfsQuota(int quota){
	syslog(LOG_DEBUG,"Writing to the file '%s'", "/cpu/cpu.cfs_quota_us");
	Util::writeFile(cgroupDirectory + "cpu/cpu.cfs_quota_us",Util::itos(quota));
	syslog(LOG_DEBUG,"'%s' has been successfully written", "/cpu/cpu.cfs_quota_us");
}


// 


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
 * Specifies the memory nodes that tasks in this cgroup are permitted to access.
 * This is a comma-separated list, with dashes to represent ranges.
 * For example: 0-2,16
 */
void Cgroup::setMems(string nodes){
	syslog(LOG_DEBUG,"Writing to the file '%s'", "/cpu/cpuset.mems");
	Util::writeFile(cgroupDirectory + "cpu/cpuset.mems",nodes);
	syslog(LOG_DEBUG,"'%s' has been successfully written", "/cpu/cpuset.mems");
}

/**
 * Contains a flag that specifies whether a page in memory should migrate to a
 * new node within the ranges if the values in cpuset.mems change. By default,
 * memory migration is disabled.
 */
void Cgroup::setMemoryMigrate(bool flag){
	syslog(LOG_DEBUG,"Writing to the file '%s'", "/cpu/cpuset.memory_migrate");
	Util::writeFile(cgroupDirectory + "cpu/cpuset.memory_migrate",flag?"1":"0");
	syslog(LOG_DEBUG,"'%s' has been successfully written", "/cpu/cpuset.memory_migrate");
}

/**
 * Contains a flag that specifies whether cpusets other than this one and its
 * parents and children can share the CPUs spceified for this cpuset. By default(0),
 * CPUs are not allocated exclusively to one cpuset
 */
void Cgroup::setCPUExclusive(bool flag){
	syslog(LOG_DEBUG,"Writing to the file '%s'", "/cpu/cpuset.cpu_exclusive");
	Util::writeFile(cgroupDirectory + "cpu/cpuset.cpu_exclusive",flag?"1":"0");
	syslog(LOG_DEBUG,"'%s' has been successfully written", "/cpu/cpuset.cpu_exclusive");
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
 * Contains a flag that specifies whether kernel allocations of memory page and
 * buffer data should be restricted to the memory nodes specified for the cpuset.
 */
void Cgroup::setMemHardwall(bool flag){
	syslog(LOG_DEBUG,"Writing to the file '%s'", "/cpu/cpuset.mem_hardwall");
	Util::writeFile(cgroupDirectory + "cpu/cpuset.mem_hardwall",flag?"1":"0");
	syslog(LOG_DEBUG,"'%s' has been successfully written", "/cpu/cpuset.mem_hardwall");
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

/**
 * Contains a flag that specifies whether kernel slab caches for file input/output
 * operations should be spread evenly across the cpuset.
 */
void Cgroup::setMemorySpreadSlab(bool flag){
	syslog(LOG_DEBUG,"Writing to the file '%s'", "/cpu/cpuset.memory_spread_slab");
	Util::writeFile(cgroupDirectory + "cpu/cpuset.spread_slab",flag?"1":"0");
	syslog(LOG_DEBUG,"'%s' has been successfully written", "/cpu/cpuset.spread_slab");
}

/**
 * Contains a flag that specifies whether the kernel will balance loads across
 * the CPUs in the cpuset. By default(1), the kernel balances loads by moving
 * processes from overloaded CPUs to less heavily used CPUs.
 */
void Cgroup::setSchedLoadBalance(bool flag){
	syslog(LOG_DEBUG,"Writing to the file '%s'", "/cpu/cpuset.sched_load_balance");
	Util::writeFile(cgroupDirectory + "cpu/cpuset.sched_load_balance",flag?"1":"0");
	syslog(LOG_DEBUG,"'%s' has been successfully written", "/cpu/cpuset.sched_load_balance");
}
