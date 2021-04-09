/**
 * package:		Part of vpl-jail-system
 * copyright:	Copyright (C) 2021 Juan Carlos Rodríguez-del-Pino. All rights reserved.
 * license:		GNU/GPL, see LICENSE.txt or http://www.gnu.org/licenses/gpl-3.0.html
 **/

#include "basetest.h"
#include <algorithm>
#include <cassert>
#include "../src/cgroup.h"

class CGroupTest: public BaseTest {
	void testSetCgroupFileSystem(){
		Cgroup::setBaseCgroupFileSystem("/sys/fs/cgroup");
		assert(Cgroup::getBaseCgroupFileSystem() == "/sys/fs/cgroup");
		Cgroup::setBaseCgroupFileSystem("/");
		assert(Cgroup::getBaseCgroupFileSystem() == "/");
	}

	void testGetCPUAcctStat(){
		char buff[FILENAME_MAX];
		assert(getcwd(buff, FILENAME_MAX) != NULL);
		string currentWorkingDir(buff);
		Cgroup::setBaseCgroupFileSystem(currentWorkingDir);
		Cgroup cgroup("cgroup.test");
		map<string, int> result = cgroup.getCPUAcctStat();
		assert(result.find("user")->second == 36509);
		assert(result.find("system")->second == 3764);
	}

	void testGetCPUStat(){
		char buff[FILENAME_MAX];
		assert(getcwd(buff, FILENAME_MAX) != NULL);
		string currentWorkingDir(buff);
		Cgroup::setBaseCgroupFileSystem(currentWorkingDir);
		Cgroup cgroup("cgroup.test");
		map<string, int> result = cgroup.getCPUStat();
		assert(result.find("nr_throttled")->second == 0);
		assert(result.find("nr_periods")->second == 0);
		assert(result.find("throttled_time")->second == 0);
	}

	void testGetMemoryStat(){
		char buff[FILENAME_MAX];
		assert(getcwd(buff, FILENAME_MAX) != NULL);
		string currentWorkingDir(buff);
		Cgroup::setBaseCgroupFileSystem(currentWorkingDir);
		Cgroup cgroup("cgroup.test");
		map<string, long int> result = cgroup.getMemoryStat();
		assert(result.find("cache")->second == 1626644480);
		assert(result.find("shmem")->second == 26406912);
		assert(result.find("mapped_file")->second == 351842304);
		assert(result.find("pgfault")->second == 2448732);
		assert(result.find("hierarchical_memory_limit")->second == 9223372036854771712L);
	}

	void testGetCPUUsage(){
		char buff[FILENAME_MAX];
		assert(getcwd(buff, FILENAME_MAX) != NULL);
		string currentWorkingDir(buff);
		Cgroup::setBaseCgroupFileSystem(currentWorkingDir);
		Cgroup cgroup("cgroup.test");
		long int result = cgroup.getCPUUsage();
		assert(result == 406582887060L);
	}

	void testGetCPUNotify(){
		char buff[FILENAME_MAX];
		assert(getcwd(buff, FILENAME_MAX) != NULL);
		string currentWorkingDir(buff);
		Cgroup::setBaseCgroupFileSystem(currentWorkingDir);
		Cgroup cgroup("cgroup.test");
		int result = cgroup.getCPUNotify();
		assert(result == 1);
	}

	void testGetReleaseAgent(){
		char buff[FILENAME_MAX];
		assert(getcwd(buff, FILENAME_MAX) != NULL);
		string currentWorkingDir(buff);
		Cgroup::setBaseCgroupFileSystem(currentWorkingDir);
		Cgroup cgroup("cgroup.test");
		string result = cgroup.getCPUReleaseAgent();
		assert(result == "0");
	}

	void testGetCPUProcs(){
		char buff[FILENAME_MAX];
		assert(getcwd(buff, FILENAME_MAX) != NULL);
		string currentWorkingDir(buff);
		Cgroup::setBaseCgroupFileSystem(currentWorkingDir);
		Cgroup cgroup("cgroup.test");
		vector<int> pids = cgroup.getCPUProcs();
		assert(count(pids.begin(), pids.end(), 1));
		assert(count(pids.begin(), pids.end(), 4735));
		assert(count(pids.begin(), pids.end(), 4730));
		assert(count(pids.begin(), pids.end(), 8));
	}

	void testGetNetPrioID(){
		char buff[FILENAME_MAX];
		assert(getcwd(buff, FILENAME_MAX) != NULL);
		string currentWorkingDir(buff);
		Cgroup::setBaseCgroupFileSystem(currentWorkingDir);
		Cgroup cgroup("cgroup.test");
		int prioId = cgroup.getNetPrioID();
		assert(prioId == 1);
	}

	void testGetPIDs(){
		char buff[FILENAME_MAX];
		assert(getcwd(buff, FILENAME_MAX) != NULL);
		string currentWorkingDir(buff);
		Cgroup::setBaseCgroupFileSystem(currentWorkingDir);
		Cgroup cgroup("cgroup.test");
		vector<int> pids = cgroup.getPIDs();
		assert(count(pids.begin(), pids.end(), 1));
		assert(count(pids.begin(), pids.end(), 128));
		assert(count(pids.begin(), pids.end(), 3116));
		assert(count(pids.begin(), pids.end(), 1366));
	}

	void testGetNetPrioMap(){
		char buff[FILENAME_MAX];
		assert(getcwd(buff, FILENAME_MAX) != NULL);
		string currentWorkingDir(buff);
		Cgroup::setBaseCgroupFileSystem(currentWorkingDir);
		Cgroup cgroup("cgroup.test");
		map<string, int> prioMap = cgroup.getNetPrioMap();
		assert(prioMap.find("lo")->second == 0);
	}

	void testGetNetNotify(){
		char buff[FILENAME_MAX];
		assert(getcwd(buff, FILENAME_MAX) != NULL);
		string currentWorkingDir(buff);
		Cgroup::setBaseCgroupFileSystem(currentWorkingDir);
		Cgroup cgroup("cgroup.test");
		int notify = cgroup.getNetNotify();
		assert(notify == 0);
	}

	void testGetNetReleaseAgent(){
		char buff[FILENAME_MAX];
		assert(getcwd(buff, FILENAME_MAX) != NULL);
		string currentWorkingDir(buff);
		Cgroup::setBaseCgroupFileSystem(currentWorkingDir);
		Cgroup cgroup("cgroup.test");
		int releaseAgent = Util::atoi(cgroup.getNetReleaseAgent());
		assert(releaseAgent == 0);
	}

	void testGetNetProcs(){
		char buff[FILENAME_MAX];
		assert(getcwd(buff, FILENAME_MAX) != NULL);
		string currentWorkingDir(buff);
		Cgroup::setBaseCgroupFileSystem(currentWorkingDir);
		Cgroup cgroup("cgroup.test");
		vector<int> tasks = cgroup.getNetProcs();
		assert(count(tasks.begin(), tasks.end(), 1));
		assert(count(tasks.begin(), tasks.end(), 4745));
	}
	void testGetMemoryProcs(){
		char buff[FILENAME_MAX];
		assert(getcwd(buff, FILENAME_MAX) != NULL);
		string currentWorkingDir(buff);
		Cgroup::setBaseCgroupFileSystem(currentWorkingDir);
		Cgroup cgroup("cgroup.test");
		vector<int> tasks = cgroup.getMemoryProcs();
		assert(count(tasks.begin(), tasks.end(), 555));
		assert(count(tasks.begin(), tasks.end(), 7));
	}

	void testGetMemoryLimitInBytes(){
		char buff[FILENAME_MAX];
		assert(getcwd(buff, FILENAME_MAX) != NULL);
		string currentWorkingDir(buff);
		Cgroup::setBaseCgroupFileSystem(currentWorkingDir);
		Cgroup cgroup("cgroup.test");
		long int limit = cgroup.getMemoryLimitInBytes();
		assert(limit == 2147483648);
	}

	void testGetMemoryUsageInBytes(){
		char buff[FILENAME_MAX];
		assert(getcwd(buff, FILENAME_MAX) != NULL);
		string currentWorkingDir(buff);
		Cgroup::setBaseCgroupFileSystem(currentWorkingDir);
		Cgroup cgroup("cgroup.test");
		long int usage = cgroup.getMemoryUsageInBytes();
		assert(usage == 3502428160);
	}

	void testGetMemNotify(){
		char buff[FILENAME_MAX];
		assert(getcwd(buff, FILENAME_MAX) != NULL);
		string currentWorkingDir(buff);
		Cgroup::setBaseCgroupFileSystem(currentWorkingDir);
		Cgroup cgroup("cgroup.test");
		int notify = cgroup.getMemNotify();
		assert(notify == 0);
	}

	void testGetMemReleaseAgent(){
		char buff[FILENAME_MAX];
		assert(getcwd(buff, FILENAME_MAX) != NULL);
		string currentWorkingDir(buff);
		Cgroup::setBaseCgroupFileSystem(currentWorkingDir);
		Cgroup cgroup("cgroup.test");
		int releaseAgent = Util::atoi(cgroup.getMemReleaseAgent());
		assert(releaseAgent == 0);
	}

	void testGetMemoryOOMControl(){
		char buff[FILENAME_MAX];
		assert(getcwd(buff, FILENAME_MAX) != NULL);
		string currentWorkingDir(buff);
		Cgroup::setBaseCgroupFileSystem(currentWorkingDir);
		Cgroup cgroup("cgroup.test");
		map<string, int> oomControl = cgroup.getMemoryOOMControl();
		assert(oomControl.find("oom_kill_disable")->second == 0);
		assert(oomControl.find("under_oom")->second == 0);
		assert(oomControl.find("oom_kill")->second == 0);
	}

	void testSetNetPrioMap(){
		char buff[FILENAME_MAX];
		assert(getcwd(buff, FILENAME_MAX) != NULL);
		string currentWorkingDir(buff);
		Cgroup::setBaseCgroupFileSystem(currentWorkingDir);
		Cgroup cgroup("cgroup.test");
		cgroup.setNetPrioMap("eth0 1");
		map<string, int> prioMap = cgroup.getNetPrioMap();
		assert(prioMap.find("eth0")->second == 1);
	}

	void testSetNetNotify(){
		char buff[FILENAME_MAX];
		assert(getcwd(buff, FILENAME_MAX) != NULL);
		string currentWorkingDir(buff);
		Cgroup::setBaseCgroupFileSystem(currentWorkingDir);
		Cgroup cgroup("cgroup.test");
		cgroup.setNetNotify(true);
		assert(cgroup.getNetNotify() == 1);
	}

	void testSetNetReleaseAgent(){
		char buff[FILENAME_MAX];
		assert(getcwd(buff, FILENAME_MAX) != NULL);
		string currentWorkingDir(buff);
		Cgroup::setBaseCgroupFileSystem(currentWorkingDir);
		Cgroup cgroup("cgroup.test");
		cgroup.setNetReleaseAgent("0");
		assert(cgroup.getNetReleaseAgent() == "0");
	}
	void testSetNetProcs(){
		char buff[FILENAME_MAX];
		assert(getcwd(buff, FILENAME_MAX) != NULL);
		string currentWorkingDir(buff);
		Cgroup::setBaseCgroupFileSystem(currentWorkingDir);
		Cgroup cgroup("cgroup.test");
		const int netProcs = 999;
		cgroup.setNetProcs(netProcs);
		vector<int> procs = cgroup.getNetProcs();
		assert(count(procs.begin(), procs.end(), netProcs));
	}
	void testSetCPUNotify(){
		char buff[FILENAME_MAX];
		assert(getcwd(buff, FILENAME_MAX) != NULL);
		string currentWorkingDir(buff);
		Cgroup::setBaseCgroupFileSystem(currentWorkingDir);
		Cgroup cgroup("cgroup.test");
		cgroup.setCPUNotify(true);
		assert(cgroup.getCPUNotify() == 1);
	}
	void testSetCPUReleaseAgentPath(){
		char buff[FILENAME_MAX];
		assert(getcwd(buff, FILENAME_MAX) != NULL);
		string currentWorkingDir(buff);
		Cgroup::setBaseCgroupFileSystem(currentWorkingDir);
		Cgroup cgroup("cgroup.test");
		cgroup.setCPUReleaseAgentPath("0");
		assert(cgroup.getCPUReleaseAgent() == "0");
	}

	void testSetCPUProcs(){
		char buff[FILENAME_MAX];
		assert(getcwd(buff, FILENAME_MAX) != NULL);
		string currentWorkingDir(buff);
		Cgroup::setBaseCgroupFileSystem(currentWorkingDir);
		Cgroup cgroup("cgroup.test");
		const int netProcs = 999;
		cgroup.setCPUProcs(netProcs);
		vector<int> procs = cgroup.getCPUProcs();
		assert(count(procs.begin(), procs.end(), netProcs));
	}

	void testSetMemoryProcs(){
		char buff[FILENAME_MAX];
		assert(getcwd(buff, FILENAME_MAX) != NULL);
		string currentWorkingDir(buff);
		Cgroup::setBaseCgroupFileSystem(currentWorkingDir);
		Cgroup cgroup("cgroup.test");
		cgroup.setMemoryProcs(1);
		vector<int> procs = cgroup.getMemoryProcs();
		assert(count(procs.begin(), procs.end(), 1));
	}

	void testSetMemoryLimitInBytes(){
		char buff[FILENAME_MAX];
		assert(getcwd(buff, FILENAME_MAX) != NULL);
		string currentWorkingDir(buff);
		Cgroup::setBaseCgroupFileSystem(currentWorkingDir);
		Cgroup cgroup("cgroup.test");
		const int memoryLimit = 2147483648;
		cgroup.setMemoryLimitInBytes(memoryLimit);
		assert(cgroup.getMemoryLimitInBytes() == memoryLimit);
	}

	void testSetMemNotify(){
		char buff[FILENAME_MAX];
		assert(getcwd(buff, FILENAME_MAX) != NULL);
		string currentWorkingDir(buff);
		Cgroup::setBaseCgroupFileSystem(currentWorkingDir);
		Cgroup cgroup("cgroup.test");
		cgroup.setMemNotify(false);
		assert(cgroup.getMemNotify() == 0);
	}

	void testSetMemReleaseAgentPath(){
		char buff[FILENAME_MAX];
		assert(getcwd(buff, FILENAME_MAX) != NULL);
		string currentWorkingDir(buff);
		Cgroup::setBaseCgroupFileSystem(currentWorkingDir);
		Cgroup cgroup("cgroup.test");
		cgroup.setMemReleaseAgentPath("0");
		assert(cgroup.getMemReleaseAgent() == "0");
	}
public:
	string name() {
		return "CGroup class";
	}
	void launch() {
		testSetCgroupFileSystem();
		testGetCPUAcctStat();
		testGetCPUStat();
		testGetMemoryStat();
		testGetCPUUsage();
		testGetCPUNotify();
		testGetReleaseAgent();
		testGetCPUProcs();
		testGetNetPrioID();
		testGetNetNotify();
		testGetNetReleaseAgent();
		testGetPIDs();
		testGetNetPrioMap();
		testGetNetProcs();
		testGetMemoryProcs();
		testGetMemoryLimitInBytes();
		testGetMemoryUsageInBytes();
		testGetMemNotify();
		testGetMemReleaseAgent();
		testGetMemoryOOMControl();
		testSetNetPrioMap();
		testSetNetProcs();
		testSetNetNotify();
		testSetNetReleaseAgent();
		testSetCPUNotify();
		testSetCPUReleaseAgentPath();
		testSetMemoryProcs();
		testSetMemoryLimitInBytes();
		testSetMemNotify();
		testSetMemReleaseAgentPath();
		testSetCPUProcs();
	}
};

CGroupTest cgroupTest;
