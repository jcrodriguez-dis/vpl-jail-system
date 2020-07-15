/**
 * package:		Part of vpl-jail-system
 * copyright:
 * license:		GNU/GPL, see LICENSE.txt or http://www.gnu.org/licenses/gpl-3.0.html
 *
 * cgroup files:
 *
 * 		
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

	std::map<std::string, int> getCPUStat();
	long int getCPUUsage();
	int getNotify();
	string getReleaseAgent();
	string getCPUTasks();
	std::map<std::string, int> getCPUStat();
	int getCloneChildren();
	string getMemoryTasks();
	int getMemCloneChildren();
	long int getMemoryLimitInBytes();
	std::map<std::string, long int> getMemoryStat();
	int getMemSwappiness();
	long int getMemoryUsageInBytes();
	int getMemNotify();
	string getMemReleaseAgent();
	// TODO
	double getMemoryHigh();
	string getMemoryMax();
	double getMemorySwapCurrent();
	string getMemorySwapMax();
	double getIOWeight();
	std::map<std::string, string> getIOMax(int flag);
	string getPIDSMax();
	int getPIDSCurrent();

	void setCgroupType(string input);
	void setCgroupProc(string input);
	void setCgroupThreads(string input);
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
