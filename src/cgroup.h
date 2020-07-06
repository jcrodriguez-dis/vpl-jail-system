/**
 * package:		Part of vpl-jail-system
 * copyright:
 * license:		GNU/GPL, see LICENSE.txt or http://www.gnu.org/licenses/gpl-3.0.html
 *
 * params:
 *
 * 		cgroup.controllers: Lists all controllers available for the cgroup to enable
 * 		cgroup.subtree_control: Controllers can be enabled and disables by writing to this file
 * 		cpy.weight: Proportionally distributes CPU cycles. Weights are in the range [1, 10000]
 *		io.max: Limits the maximum BPS and/or IOPS that a cgroup can consume
 *		cgroup.type: Read-write single value file that indicates the current type of the cgroup
 *
 **/
#include <string>

class Cgroup {
private:
	static Cgroup* singlenton;
	Cgroup();
	string readCgroupFile(string filename);
	void writeCgroupFile(string filename, string input);
public:
	static Cgroup* getCgroup(){
		if(singlenton == NULL) singlenton = new Cgroup();
		return singlenton;
	}

	string readCgroupType();
	string readCgroupProcs();
	string readCgroupThreads();
	string readCgroupControllers();
	string readCgroupEvents();
	string readCgroupMaxDescendants();
	string readCgroupStat();
	string readCPUStat();
	string readCPUWeight();
	string readCPUMax();
	string readMemoryCurrent();
	string readMemoryLow();
	string readMemoryHigh();
	string readMemoryMax();
	string readMemorySwapCurrent();
	string readMemorySwapMax();
	string readIOWeight();
	string readIOMax();
	string readPIDSMax();
	string readPIDSCurrent();

	void writeCgroupType(string input);
	void writeCgroupProcs(string input);
	void writeCgroupThreads(string input);
	void writeCgroupSubtreeControl(string input);
	void writeCgroupMaxDesscendants(string input);
	void writeCgroupMaxDepth(string input);
	void writeCPUWeight(string input);
	void writeCPUMax(string input);
	void writeMemoryLow(string input);
	void writeMemoryHigh(string input);
	void writeMemoryMax(string input);
	void writeMemorySwapMax(string input);
	void writeIOWeight(string input);
	void writeIOMax(string input);
	void writePIDSMax(string input);

};
