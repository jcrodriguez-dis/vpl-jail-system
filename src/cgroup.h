/**
 * package:		Part of vpl-jail-system
 * copyright:
 * license:		GNU/GPL, see LICENSE.txt or http://www.gnu.org/licenses/gpl-3.0.html
 *
 * cgroup files:
 *
 * 		cgroup.controllers: Lists all controllers available for the cgroup to enable
 * 		cgroup.subtree_control: Controllers can be enabled and disables by writing to this file
 * 		cpu.weight: Proportionally distributes CPU cycles. Weights are in the range [1..10000]
 *		io.max: Limits the maximum BPS and/or IOPS that a cgroup can consume
 *		cgroup.type: Read-write single value file that indicates the current type of the cgroup
 *		cpu.stat
 *		cpu.max
 *		memory.current
 *		memory.low
 *		memory.high
 *		memory.max
 *		memory.swap.current
 *		memory.swap.max
 *		io.weight
 *		pids.max
 *		pids.current
 *
 **/

#include <string>
#include "util.h"
#include <syslog.h>
#include <iostream>
#include <map>

class Cgroup {
private:
	static string baseCgroupFilesystem;
	string cgroupDirectory;
	string readCgroupFile(string filename);
	void writeCgroupFile(string filename, string input);
public:
	static void setBaseCgroupFilesystem(string fs);
	Cgroup(string name){
		cgroupDirectory = baseCgroupFilesystem + "/" + name + "/";
	}

	string getgroupType();
	int getCgroupProcs();
	string getCgroupThreads();
	string getCgroupControllers();
	string getCgroupEvents();
	string getCgroupMaxDescendants();
	std::map<std::string, int> getCgroupStat();
	std::map<std::string, int> getCPUStat(int flag);
	double getCPUWeight();
	string getCPUMax();
	double getMemoryCurrent();
	double getMemoryLow();
	double getMemoryHigh();
	string getMemoryMax();
	double getMemorySwapCurrent();
	string getMemorySwapMax();
	double getIOWeight();
	std::map<std::string, string> getIOMax();
	string getPIDSMax();
	int getPIDSCurrent();

	void setCgroupType(string input);
	void setCgroupProc(int input);
	void setCgroupThreads(string int);
	void setCgroupSubtreeControl(string input);
	void setCgroupMaxDesscendants(string input);
	void setCgroupMaxDepth(string input);
	void setCPUWeight(string input);
	void setCPUMax(string input);
	void setMemoryLow(string input);
	void setMemoryHigh(string input);
	void setMemoryMax(string input);
	void setMemorySwapMax(string input);
	void setIOWeight(string input);
	void setIOMax(string input);
	void setPIDSMax(string input);

};
