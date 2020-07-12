/**
 * package:		Part of vpl-jail-system
 * copyright:
 * license:		GNU/GPL, see LICENSE.txt or http://www.gnu.org/licenses/gpl-3.0.html
 **/

#include "cgroup.h"

using namespace std;

string getCgroupType(){
	string type;
	syslog(LOG_DEBUG, "Reading from the file '%s'", "cgroup.type");
	type = Util::readFile(cgroupDirectory + "cgroup.type");
	return type;
}

int getCgroupProc(){
	int proc;
	syslog(LOG_DEBUG, "Reading from the file '%s'", "cgroup.procs");
	proc = (int) Util::readFile(cgroupDirectory + "cgroup.procs");
	return proc;
}

string getCgroupThreads(){
	string threads;
	syslog(LOG_DEBUG, "Reading from the file '%s'", "cgroup.threads");
	threads = Util::readFile(cgroupDirectory + "cgroup.threads");
	return threads;
}

string getCgroupControllers(){
	string controllers;
	syslog(LOG_DEBUG, "Reading from the file '%s'", "cgroup.controllers");
	controllers = Util::readFile(cgroupDirectory + "cgroup.controllers");
	return controllers;
}

string getCgroupEvents(){
	string events;
	syslog(LOG_DEBUG, "Reading from the file '%s'", "cgroup.events");
	events = Util::readFile(cgroupDirectory + "cgroup.events");
	return events;
}

string getCgroupMaxDescendants(){
	string maxDesc;
	syslog(LOG_DEBUG, "Reading from the file '%s'", "cgroup.max.descendants");
	maxDesc = Util::readFile(cgroupDirectory + "cgroup.max.descendants");
	return maxDesc;
}

string getCgroupMaxDepth(){
	string maxDepth;
	syslog(LOG_DEBUG, "Reading from the file '%s'", "cgroup.max.depth");
	maxDepth = Util::readFile(cgroupDirectory + "cgroup.max.depth");
	return maxDepth;
}

std::map<std::string, int> getCgroupStat(){
	string stat;
	syslog(LOG_DEBUG, "Reading from the file '%s'", "cgroup.stat");
	stat = Util::readFile(cgroupDirectory + "cgroup.stat");
	int descendants = stat.substr(cpu.find(" "), stat.find("\n"));
	int dying = stat.substr(stat.find("nr_dying_descendants")+21, stat.length());
	std::map<string,int> descendantInfo;
	descendantInfo.insert(std::pair<string,int>("nr_descendants", descendants));
	descendantInfo.insert(std::pair<string,int>("nr_dying_descendants", dying));
	return descendantInfo;
}

/**
 * parameter flag:
 * 0 = get all cpu usage info
 * 1 = get total cpu usage
 * 2 = get user cpu usage
 * 3 = get system cpu usage
 */

std::map<std::string, int> getCPUStat(int flag){
	string cpu;
	syslog(LOG_DEBUG, "Reading from the file '%s'", "cgroup.cpu.stat");
	cpu = Util::readFile(cgroupDirectory + "cgroup.cpu.stat");
	int total = cpu.substr(cpu.find(" "), cpu.find("\n"));
	int user = cpu.substr(cpu.find("user_usec")+10, cpu.find("system_usec"));
	int system = cpu.substr(cpu.find("system_usec")+12, cpu.length());
	std::map<string,int> cpuInfo;
	switch (flag) {
	case 0:
		cpuInfo.insert(std::pair<string,int>("usage_usec", total));
		cpuInfo.insert(std::pair<string,int>("user_usec", user));
		cpuInfo.insert(std::pair<string,int>("system_usec", system));
		break;
	case 1:
		cpuInfo.insert(std::pair<string,int>("usage_usec", total));
		break;
	case 2:
		cpuInfo.insert(std::pair<string,int>("user_usec", user));
		break;
	case 3:
		cpuInfo.insert(std::pair<string,int>("system_usec", system));
		break;
	default:
		break;
	}		
	return cpuInfo;
}

double getCPUWeight(){
	double weight;
	syslog(LOG_DEBUG, "Reading from the file '%s'", "cgroup.cpu.weight");
	weight = (double) Util::readFile(cgroupDirectory + "cgroup.cpu.weight");
	return weight;
}

string getCPUMax(){
	string cpuMax;
	syslog(LOG_DEBUG, "Reading from the file '%s'", "cgroup.cpu.max");
	cpuMax = Util::readFile(cgroupDirectory + "cgroup.cpu.max");
	return cpuMax;
}

double getMemoryCurrent(){
	double currentMem;
	cout << "Reading from the file cgroup.memory.current" << endl;
	currentMem = (double) Util::readFile(cgroupDirectory + "cgroup.memory.current");
	return currentMem;
}

double getMemoryLow(){
	double low;
	syslog(LOG_DEBUG, "Reading from the file '%s'", "cgroup.memory.low");
	low = (double) Util::readFile(cgroupDirectory + "cgroup.memory.low");
	return low;
}

string getMemoryHigh(){
	string high;
	syslog(LOG_DEBUG, "Reading from the file '%s'", "cgroup.memory.high");
	high = Util::readFile(cgroupDirectory + "cgroup.memory.high");
	return high;
}

string getMemoryMax(){
	string maxMem;
	syslog(LOG_DEBUG, "Reading from the file '%s'", "cgroup.memory.max");
	maxMem = Util::readFile(cgroupDirectory + "cgroup.memory.max");
	return maxMem;
}

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
	string rbps = cpu.substr(cpu.find("rbps")+5, cpu.find(" "));
	string wbps = cpu.substr(cpu.find("wbps")+5, cpu.find(" "));
	string riops = cpu.substr(cpu.find("riops")+6, cpu.find(" "));
	string wiops = cpu.substr(cpu.find("wiops")+5, cpu.find(" "));
	std::map<string,int> ioInfo;
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
