/**
 * package:		Part of vpl-jail-system
 * copyright:
 * license:		GNU/GPL, see LICENSE.txt or http://www.gnu.org/licenses/gpl-3.0.html
 **/

#include "cgroup.h"

using namespace std;

std::map<std::string, int> getCPUAcctStat(){
	std::map<std::string, int> cpuStat;
	string stat;
	syslog(LOG_DEBUG, "Reading from the file '%s'", cgroupDirectory + "/cpu/cpuacct.stat");
	stat = Util::readFile(cgroupDirectory + "/cpu/cpuacct.stat");
	int user = Util::atoi(stat.substr(cpu.find("user")+5, stat.find("\n")));
	int system = Util::atoi(stat.substr(stat.find("system")+7, stat.length()));
	cpuStat.insert(std::pair<string,int>("user", user));
	cpuStat.insert(std::pair<string,int>("system", system));
	return cpuStat;
}

long int getCPUUsage(){
	long int usage;
	syslog(LOG_DEBUG, "Reading from the file '%s'", cgroupDirectory + "/cpu/cpuacct.usage");
	usage = Util::atol(Util::readFile(cgroupDirectory + "/cpu/cpuacct.usage"));
	return usage;
}

int getNotify(){
	int flag;
	syslog(LOG_DEBUG, "Reading from the file '%s'", cgroupDirectory + "/cpu/notify_on_release");
	threads = Util::readFile(cgroupDirectory + "/cpu/notify_on_release");
	return flag;
}

string getReleaseAgent(){
	string releaseAgent;
	syslog(LOG_DEBUG, "Reading from the file '%s'", cgroupDirectory + "/cpu/release_agent");
	releaseAgent = Util::readFile(cgroupDirectory + "/cpu/release_agent");
	return releaseAgent;
}

string getCPUTasks(){
	string tasks;
	syslog(LOG_DEBUG, "Reading from the file '%s'", cgroupDirectory + "/cpu/tasks");
	events = Util::readFile(cgroupDirectory + "/cpu/tasks");
	return tasks;
}

std::map<std::string, int> getCPUStat(){
	std::map<std::string, int> cpuStat;
	syslog(LOG_DEBUG, "Reading from the file '%s'", cgroupDirectory + "/cpu/cpu.stat");
	string stat = Util::readFile(cgroupDirectory + "/cpu/cpu.stat");
	int nrPeriods = Util::atoi(stat.substr(cpu.find("nr_periods")+11, stat.find("\n")));
	int nrThrottled = Util::atoi(stat.substr(stat.find("nr_throttled")+13, stat.find("\n")));
	int throttledTime = Util::atoi(stat.substr(stat.find("throttled_time")+15, stat.length()));
	cpuStat.insert(std::pair<string,int>("nr_periods", nrPeriods));
	cpuStat.insert(std::pair<string,int>("nr_throttled", nrThrottled));
	cpuStat.insert(std::pair<string,int>("throttled_time", throttledTime));
	return cpuStat;
}

int getCloneChildren(){
	int clone;
	syslog(LOG_DEBUG, "Reading from the file '%s'", cgroupDirectory + "/cpu/cgroup.clone_children");
	clone = Util::atoi(Util::readFile(cgroupDirectory + "/cpu/cgroup.clone_children"));
	return clone;
}

string getMemoryTasks(){
	string tasks;
	syslog(LOG_DEBUG, "Reading from the file '%s'", "/memory/tasks");
	tasks = Util::readFile(cgroupDirectory + "/memory/tasks");	
	return tasks;

int getMemCloneChildren(){
	int clone;
	syslog(LOG_DEBUG, "Reading from the file '%s'", cgroupDirectory + "/cpu/cgroup.clone_children");
	clone = Util::atoi(Util::readFile(cgroupDirectory + "/cpu/cgroup.clone_children"));
	return clone;
	}

long int getMemoryLimitInBytes(){
	long int memLimit;
	syslog(LOG_DEBUG, "Reading from the file '%s'", cgroupDirectory + "/memory/memory.limit_in_bytes");
	weight = Util::atol(Util::readFile(cgroupDirectory + "/memory/memory.limit_in_bytes"));
	return memLimit;
}

std::map<std::string, long int> getMemoryStat(){
	std::map<std::string, int> memStat;
	string stat;
	syslog(LOG_DEBUG, "Reading from the file '%s'", cgroupDirectory + "/memory/memory.stat");
	stat = Util::readFile(cgroupDirectory + "/memory/memory.stat");
	long int cache = Util::atoi(stat.substr(cpu.find("cache")+6, stat.find("\n")));
	long int shmem = Util::atoi(stat.substr(stat.find("shmem")+6, stat.find("\n")));
	long int mappedFile = Util::atoi(stat.substr(stat.find("mapped_file")+12, stat.find("\n")));
	long int pgfault = Util::atoi(stat.substr(cpu.find("pgfault")+8, stat.find("\n")));
	long int hierarchicalLimit = Util::atoi(stat.substr(stat.find("hierarchical_memory_limit")+26, stat.find("\n")));	
	memStat.insert(std::pair<string,long int>("cache", cache));
	memStat.insert(std::pair<string,long int>("shmem", shmem));
	memStat.insert(std::pair<string,long int>("mapped_file", mappedFile));
	memStat.insert(std::pair<string,long int>("pgfault", pgfault));
	memStat.insert(std::pair<string,long int>("hierarchical_memory_limit", hierarchicalLimit));
	return memStat;
}

int getMemSwappiness(){
	int memSwap;
	syslog(LOG_DEBUG, "Reading from the file '%s'", cgroupDirectory + "/memory/memory.swappiness");
	memSwap = Util::atoi(Util::readFile(cgroupDirectory + "/memory/memory.swappiness"));
	return memSwap;
}

long int getMemoryUsageInBytes(){
	long int memUsage;
	syslog(LOG_DEBUG, "Reading from the file '%s'", cgroupDirectory + "/memory/memory.usage_in_bytes);
	memUsage = Util::atol(Util::readFile(cgroupDirectory + "/memory/memory.usage_in_bytes"));
	return memUsage;
}

int getMemNotify(){
	int flag;
	syslog(LOG_DEBUG, "Reading from the file '%s'", cgroupDirectory + "/memory/notify_on_release");
	flag = Util::atoi(Util::readFile(cgroupDirectory + "/memory/notify_on_release"));
	return flag;
}

string getMemReleaseAgent(){
	string releaseAgent;
	syslog(LOG_DEBUG, "Reading from the file '%s'", cgroupDirectory + "/memory/release_agent");
	releaseAgent = Util::readFile(cgroupDirectory + "/memory/release_agent");
	return releaseAgent;
}

//TODO 

double getMemorySwapCurrent(){
	double currentSwap;
	syslog(LOG_DEBUG, "Reading from the file '%s'", "cgroup.swap.current");
	currentSwap = (double) Util::readFile(cgroupDirectory + "cgroup.swap.current");
	return currentSwap;
}

string getMemorySwapMax(){
	string maxSwap;
	syslog(LOG_DEBUG, "Reading from the file '%s'", "cgroup.swap.max");
	maxSwap = Util::readFile(cgroupDirectory + "cgroup.swap.max");
	return maxSwap;
}

double getIOWeight(){
	string ioWeight;
	syslog(LOG_DEBUG, "Reading from the file '%s'", "cgroup.io.weight");
	ioWeight = Util::readFile(cgroupDirectory + "cgroup.io.weight");
	return ioWeight;
}

/**
 * parameter flag:
 * 
 * 0 = get all information
 * 1 = get max read bytes per second
 * 2 = get max write bytes per second
 * 3 = get max read IO operations per second
 * 4 = get max write IO operations per second
 */
std::map<string,string> getIOMax(int flag){
	string ioMax;
	syslog(LOG_DEBUG, "Reading from the file '%s'", "cgroup.io.max");
	ioMax = Util::readFile(cgroupDirectory + "cgroup.io.max");
	string rbps = cpu.substr(cpu.find("rbytes")+5, cpu.find(" "));
	string wbps = cpu.substr(cpu.find("wbytes")+5, cpu.find(" "));
	string riops = cpu.substr(cpu.find("rios")+6, cpu.find(" "));
	string wiops = cpu.substr(cpu.find("wios")+5, cpu.find(" "));
	std::map<string,string> ioInfo;
	switch (flag){
	case 0:
		ioInfo.insert(std::pair<string,string>("rbps", rbps));
		ioInfo.insert(std::pair<string,string>("wbps", wbps));
		ioInfo.insert(std::pair<string,string>("riops", riops));
		ioInfo.insert(std::pair<string,string>("wiops", wiops));
	case 1:
		ioInfo.insert(std::pair<string,string>("rbps", rbps));
	case 2:
		ioInfo.insert(std::pair<string,string>("wbps", wbps));
	case 3:
		ioInfo.insert(std::pair<string,string>("riops", riops));
	case 4:
		ioInfo.insert(std::pair<string,string>("wiops", wiops));
	default:
		break;
	}
	return ioInfo;
}

string getPIDSMax(){
	string pidsMax;
	syslog(LOG_DEBUG, "Reading from the file '%s'", "cgroup.pids.max");
	pidsMax = Util::readFile(cgroupDirectory + "cgroup.pids.max");
	return pidsMax;
}

int getPIDSCurrent(){
	string currentPids;
	syslog(LOG_DEBUG, "Reading from the file '%s'", "cgroup.pids.current");
	currentPids = Util::readFile(cgroupDirectory + "cgroup.pids.current");
	return currentPids;
}

/**
 * option:
 * - "threaded": Turn the cgroup into a member of a threaded subtree
 */
void setCgroupType(string input){
	syslog(LOG_DEBUG,"Writing to the file '%s'", "cgroup.type");
	Util::writeFile(cgroupDirectory + "cgroup.type",input);
	syslog(LOG_DEBUG,"'%s' has been successfully written", "cgroup.type");
}

/**
 * param: Insert PID of desired process
 */
void setCgroupProcs(string input){
	syslog(LOG_DEBUG,"Writing to the file '%s'", "cgroup.procs");
	Util::writeFile(cgroupDirectory + "cgroup.procs",input);
	syslog(LOG_DEBUG,"'%s' has been successfully written", "cgroup.procs");
}
/**
 * param: Insert TID of desired thread
 */
void setCgroupThreads(int input){
	syslog(LOG_DEBUG,"Writing to the file '%s'", "cgroup.threads");
	Util::writeFile(cgroupDirectory + "cgroup.threads",input);
	syslog(LOG_DEBUG,"'%s' has been successfully written", "cgroup.threads");
}
/**
 * param: Space separated list of controllers prefixed with '+' or '-' can be written
 * to enable or disable controllers. Controllers are 'cpu', 'memory' and 'io'
 * E.g: "+cpu +memory -io"
 */
void setCgroupSubtreeControl(string input){
	syslog(LOG_DEBUG,"Writing to the file '%s'", "cgroup.subtree_control");
	Util::writeFile(cgroupDirectory + "cgroup.subtree_control",input);
	syslog(LOG_DEBUG,"'%s' has been successfully written", "cgroup.subtree_control");
}
void setCgroupMaxDesscendants(string input){
	syslog(LOG_DEBUG,"Writing to the file '%s'", "cgroup.max.descendants");
	Util::writeFile(cgroupDirectory + "cgroup.max.descendants",input);
	syslog(LOG_DEBUG,"'%s' has been successfully written", "cgroup.max.descendants");
}
void setCgroupMaxDepth(string input){
	syslog(LOG_DEBUG,"Writing to the file '%s'", "cgroup.max.depth");
	Util::writeFile(cgroupDirectory + "cgroup.max.depth",input);
	syslog(LOG_DEBUG,"'%s' has been successfully written", "max.depth");
}
void setCPUWeight(string input){
	syslog(LOG_DEBUG,"Writing to the file '%s'", "cgroup.cpu.weight");
	Util::writeFile(cgroupDirectory + "cgroup.cpu.weight",input);
	syslog(LOG_DEBUG,"'%s' has been successfully written", "cgroup.cpu.weight");
}
void setCPUMax(string input){
	syslog(LOG_DEBUG,"Writing to the file '%s'", "cgroup.cpu.max");
	Util::writeFile(cgroupDirectory + "cgroup.cpu.max",input);
	syslog(LOG_DEBUG,"'%s' has been successfully written", "cgroup.cpu.max");
}
void setMemoryLow(string input){
	syslog(LOG_DEBUG,"Writing to the file '%s'", "cgroup.memory.low");
	Util::writeFile(cgroupDirectory + "cgroup.memory.low",input);
	syslog(LOG_DEBUG,"'%s' has been successfully written", "cgroup.memory.low");
}
void setMemoryHigh(string input){
	syslog(LOG_DEBUG,"Writing to the file '%s'", "cgroup.memory.high");
	Util::writeFile(cgroupDirectory + "cgroup.memory.high",input);
	syslog(LOG_DEBUG,"'%s' has been successfully written", "cgroup.memory.high");
}
void setMemoryMax(string input){
	syslog(LOG_DEBUG,"Writing to the file '%s'", "cgroup.memory.max");
	Util::writeFile(cgroupDirectory + "cgroup.memory.max",input);
	syslog(LOG_DEBUG,"'%s' has been successfully written", "cgroup.memory.max");
}
void setMemorySwapMax(string input){
	syslog(LOG_DEBUG,"Writing to the file '%s'", "cgroup.swap.max");
	Util::writeFile(cgroupDirectory + "cgroup.swap.max",input);
	syslog(LOG_DEBUG,"'%s' has been successfully written", "cgroup.swap.max");
}
void setIOWeight(string input){
	syslog(LOG_DEBUG,"Writing to the file '%s'", "cgroup.io.weight");
	Util::writeFile(cgroupDirectory + "cgroup.io.weight",input);
	syslog(LOG_DEBUG,"'%s' has been successfully written", "cgroup.io.weight");
}
void writeIOMax(string input){
	syslog(LOG_DEBUG,"Writing to the file '%s'", "cgroup.io.max");
	Util::writeFile(cgroupDirectory + "cgroup.io.max",input);
	syslog(LOG_DEBUG,"'%s' has been successfully written", "cgroup.io.max");
}
void writePIDSMax(string input){
	syslog(LOG_DEBUG,"Writing to the file '%s'", "cgroup.pids.max");
	Util::writeFile(cgroupDirectory + "cgroup.pids.max",input);
	syslog(LOG_DEBUG,"'%s' has been successfully written", "cgroup.pids.max");
}
