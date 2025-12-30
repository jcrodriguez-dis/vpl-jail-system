/**
 * package:		Part of vpl-jail-system
 * copyright:
 * license:		GNU/GPL, see LICENSE.txt or http://www.gnu.org/licenses/gpl-3.0.html
 **/

#include "cgroup.h"
#include "util.h"

using namespace std;

string Cgroup::baseCgroupFileSystem= Util::findCgroupFilesystem();
bool Cgroup::baseCgroupFileSystemOverridden = false;
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
const char* Cgroup::FILE_CPU_TASKS = "cpu,cpuacct/tasks";
const char* Cgroup::FILE_CPU_STAT = "cpu,cpuacct/cpu.stat";
const char* Cgroup::FILE_NET_TASKS = "net_cls,net_prio/tasks";

const char* Cgroup::FILE_PIDS_PROCS = "pids/cgroup.procs";

const char* Cgroup::FILE_MEM_TASKS = "memory/tasks";
const char* Cgroup::FILE_MEM_LIMIT = "memory/memory.limit_in_bytes";
const char* Cgroup::FILE_MEM_STAT = "memory/memory.stat";
const char* Cgroup::FILE_MEM_USAGE = "memory/memory.usage_in_bytes";
const char* Cgroup::FILE_MEM_OOM_CONTROL = "memory/memory.oom_control";
bool Cgroup::isV2 = false;

void Cgroup::init() {
    static bool initialized = false;
    if (!initialized) {
        if (!Cgroup::baseCgroupFileSystemOverridden) {
            Cgroup::baseCgroupFileSystem = Util::findCgroupFilesystem();
        }
        Cgroup::isV2 = Cgroup::isCgroupV2Base(Cgroup::baseCgroupFileSystem);
        Cgroup::enableV2Controllers();
        initialized = true;
    }
}   

bool Cgroup::isCgroupV2Base(string base) {
    // cgroup v2 has a unified hierarchy and always exposes cgroup.controllers at the mount root.
    return Util::fileExists(joinPath(base, "cgroup.controllers"), true);
}

static long long parseCpuStatValue(const string &stat, const string &key) {
    std::istringstream iss(stat);
    string k;
    long long v;
    while (iss >> k >> v) {
        if (k == key) {
            return v;
        }
    }
    return 0;
}

void Cgroup::createV2Cgroup() {
    if (!isV2) {
        return;
    }
    if (mkdir(cgroupDirectory.c_str(), 0755) != 0) {
        if (errno != EEXIST) {
            Logger::log(LOG_WARNING, "Failed to create cgroup v2 directory %s: %s (errno=%d)",
                    cgroupDirectory.c_str(), strerror(errno), errno);
        }
    }
}

void Cgroup::enableV2Controllers() {
    if (!isV2) {
        return;
    }
    // Best-effort: try to enable controllers in the parent so files exist for children.
    // This can legitimately fail on systemd-managed or busy hierarchies; we continue anyway.
    const string parent = Cgroup::getBaseCgroupFileSystem();
    const string controllersFile = parent + "/cgroup.controllers";
    const string subtreeFile = parent + "/cgroup.subtree_control";
    string controllers;
    try {
        controllers = Util::readFile(controllersFile, false);
    } catch (...) {
        controllers = "";
    }
    if (!controllers.empty()) {
        const char *want[] = {"memory", "pids", "cpu"};
        for (int i = 0; i < 3; i++) {
            string token = want[i];
            if (controllers.find(token) == string::npos) {
                continue;
            }
            try {
                // Writing a single token is allowed; kernel merges tokens.
                Util::writeFile(subtreeFile, "+" + token);
            } catch (...) {
                // Non-fatal.
            }
        }
    }
}

void Cgroup::attachPidV2(int pid) {
    const string path = v2Path("cgroup.procs");
    Logger::log(LOG_DEBUG, "Writing to the file '%s'", path.c_str());
    // Overwrite is fine; kernel interprets the number and moves the process.
    Util::writeFile(path, Util::itos(pid));
}

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
    if (isV2) {
        // v2: cpu.stat provides user_usec/system_usec.
        const string path = v2Path("cpu.stat");
        Logger::log(LOG_DEBUG, "Reading from the file '%s'", path.c_str());
        string cpu = Util::readFile(path);
        long long userUsec = parseCpuStatValue(cpu, "user_usec");
        long long systemUsec = parseCpuStatValue(cpu, "system_usec");
        long long hz = sysconf(_SC_CLK_TCK);
        if (hz <= 0) hz = 100;
        // cpuacct.stat reports USER_HZ ticks; approximate from usec.
        long long userTicks = (userUsec * hz) / 1000000LL;
        long long systemTicks = (systemUsec * hz) / 1000000LL;
        cpuStat["user"] = (userTicks > std::numeric_limits<int>::max()) ? std::numeric_limits<int>::max() : (int)userTicks;
        cpuStat["system"] = (systemTicks > std::numeric_limits<int>::max()) ? std::numeric_limits<int>::max() : (int)systemTicks;
        return cpuStat;
	}
    string path = v1Path(FILE_CPU_ACCT_STAT);
    Logger::log(LOG_DEBUG, "Reading from the file '%s'", path.c_str());
    stat = Util::readFile(path);

    string sUser = regFound(regUser, stat);
    string sSystem = regFound(regSystem, stat);

    cpuStat["user"] = Util::atoi(sUser);
    cpuStat["system"] = Util::atoi(sSystem);
    return cpuStat;
}


long int Cgroup::getCPUUsage(){
    if (isV2) {
        // v2: cpu.stat usage_usec. cpuacct.usage is nanoseconds; convert usec->nsec.
        const string path = v2Path("cpu.stat");
        Logger::log(LOG_DEBUG, "Reading from the file '%s'", path.c_str());
        string cpu = Util::readFile(path);
        long long usageUsec = parseCpuStatValue(cpu, "usage_usec");
        long long nsec = usageUsec * 1000LL;
        if (nsec > std::numeric_limits<long int>::max()) {
            return std::numeric_limits<long int>::max();
        }
        return (long int)nsec;
	}
    string path = v1Path(FILE_CPU_USAGE);
    Logger::log(LOG_DEBUG, "Reading from the file '%s'", path.c_str());
    return Util::atol(Util::readFile(path));
}

vector<int> Cgroup::getCPUProcs(){
    string path;
    if (isV2) {
        path = v2Path("cgroup.procs");
    } else {
        path = v1Path(FILE_CPU_TASKS);
    }
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
    string path;
    if (isV2) {
		path = v2Path("cpu.stat");
	} else {
		path = v1Path(FILE_CPU_STAT);
	}
    Logger::log(LOG_DEBUG, "Reading from the file '%s'", path.c_str());
    string stat = Util::readFile(path);
	if (isV2) {
		cpuStat["nr_periods"] = (int)parseCpuStatValue(stat, "nr_periods");
		cpuStat["nr_throttled"] = (int)parseCpuStatValue(stat, "nr_throttled");
		// v1 uses throttled_time (nsec), v2 uses throttled_usec. Convert to nsec.
		long long thrUsec = parseCpuStatValue(stat, "throttled_usec");
		long long thrNsec = thrUsec * 1000LL;
		cpuStat["throttled_time"] = (thrNsec > std::numeric_limits<int>::max()) ? std::numeric_limits<int>::max() : (int)thrNsec;
		return cpuStat;
	}

    string nrPeriods = regFound(regPeriods, stat);
    string nrThrottled = regFound(regThrottled, stat);
    string throttledTime = regFound(regThrottledTime, stat);

    cpuStat["nr_periods"] = Util::atoi(nrPeriods);
    cpuStat["nr_throttled"] = Util::atoi(nrThrottled);
    cpuStat["throttled_time"] = Util::atoi(throttledTime);
    return cpuStat;
}

vector<int> Cgroup::getPIDs(){
    string path;
    if (isV2) {
        path = v2Path("cgroup.procs");
    } else {
        path = v1Path(FILE_PIDS_PROCS);
    }
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

vector<int> Cgroup::getNetProcs(){
    if (isV2) {
		return getCPUProcs();
	}
    string path = v1Path(FILE_NET_TASKS);
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
    string path;
    if (isV2) {
        path = v2Path("cgroup.procs");
    } else {
        path = v1Path(FILE_MEM_TASKS);
    }
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
    string path;
    if (isV2) {
        path = v2Path("memory.max");
        Logger::log(LOG_DEBUG, "Reading from the file '%s'", path.c_str());
        string raw = Util::readFile(path);
        raw = regFound(regTrim, raw);
        if (raw == "max") {
            return std::numeric_limits<long int>::max();
        }
        return Util::atol(raw);
    }
    path = v1Path(FILE_MEM_LIMIT);
    Logger::log(LOG_DEBUG, "Reading from the file '%s'", path.c_str());
    return Util::atol(Util::readFile(path));
}

map<string, long int> Cgroup::getMemoryStat(){
    map<string, long int> memStat;
    string stat;
    if (isV2) {
        string path = v2Path("memory.stat");
        Logger::log(LOG_DEBUG, "Reading from the file '%s'", path.c_str());
        stat = Util::readFile(path);
        // v2 memory.stat is key value per-line.
        // Keep the existing API keys when we can.
        std::istringstream iss(stat);
        string key;
        long long value;
        long long fileCache = 0;
        long long shmem = 0;
        long long mappedFile = 0;
        long long pgfault = 0;
        while (iss >> key >> value) {
            if (key == "file") fileCache = value;
            else if (key == "shmem") shmem = value;
            else if (key == "mapped_file") mappedFile = value;
            else if (key == "pgfault") pgfault = value;
        }
        memStat["cache"] = (long int)fileCache;
        memStat["shmem"] = (long int)shmem;
        memStat["mapped_file"] = (long int)mappedFile;
        memStat["pgfault"] = (long int)pgfault;
        memStat["hierarchical_memory_limit"] = getMemoryLimitInBytes();
        return memStat;
    }
    string path = v1Path(FILE_MEM_STAT);
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
    string path;
    if (isV2) {
        path = v2Path("memory.current");
    } else {
        path = v1Path(FILE_MEM_USAGE);
    }
    Logger::log(LOG_DEBUG, "Reading from the file '%s'", path.c_str());
    return Util::atol(Util::readFile(path));
}

map<string, int> Cgroup::getMemoryOOMControl(){
    map<string, int> memoryOOM;
    if (isV2) {
        // v2 exposes OOM info via memory.events. There is no oom_kill_disable/under_oom.
        memoryOOM["oom_kill_disable"] = 0;
        memoryOOM["under_oom"] = 0;
        memoryOOM["oom_kill"] = 0;
        try {
            string path = v2Path("memory.events");
            Logger::log(LOG_DEBUG, "Reading from the file '%s'", path.c_str());
            string events = Util::readFile(path);
            std::istringstream iss(events);
            string key;
            long long value;
            while (iss >> key >> value) {
                if (key == "oom_kill") {
                    memoryOOM["oom_kill"] = (int)value;
                }
            }
        } catch (...) {
            // Best-effort.
        }
        return memoryOOM;
    }
    string path = v1Path(FILE_MEM_OOM_CONTROL);
    Logger::log(LOG_DEBUG, "Reading from the file '%s'", path.c_str());
    string stat = Util::readFile(path);
    string oomDisable = regFound(regOOM, stat);
    string underOOM = regFound(regUnder, stat);
    string oomKill = regFound(regKill, stat);
    memoryOOM["oom_kill_disable"] = Util::atoi(oomDisable);
    memoryOOM["under_oom"] = Util::atoi(underOOM);
    memoryOOM["oom_kill"] = Util::atoi(oomKill);
    return memoryOOM;
}

void Cgroup::setNetProcs(int pid){
    if (isV2) {
		attachPidV2(pid);
		return;
	}
    string path = v1Path(FILE_NET_TASKS);
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
 * @param pid Process ID
 */
void Cgroup::setCPUProcs(int pid){
    if (isV2) {
		attachPidV2(pid);
		return;
	}
    string path = v1Path(FILE_CPU_TASKS);
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
 * Insert a process' PID to allow it to be in the memory controllers
 */
void Cgroup::setMemoryProcs(int pid){
    if (isV2) {
		attachPidV2(pid);
		return;
	}
    string path = v1Path(FILE_MEM_TASKS);
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
    string path;
    if (isV2) {
        path = v2Path("memory.max");
    } else {
        path = v1Path(FILE_MEM_LIMIT);
    }
    Logger::log(LOG_DEBUG,"Writing to the file '%s'", path.c_str());
    Util::writeFile(path, Util::itos(bytes));
    Logger::log(LOG_DEBUG,"'%s' has been successfully written", path.c_str());
}

/**
 * Remove this cgroup from all controllers
 * Removes the cgroup directories from cpu, memory, and network controllers
 * All processes must be moved out before calling this
 */
void Cgroup::removeCgroup(){
    if (isV2) {
        // v2 has a single directory per cgroup.
        Logger::log(LOG_DEBUG, "Removing cgroup v2 directory: %s", cgroupDirectory.c_str());
        if (rmdir(cgroupDirectory.c_str()) == 0) {
            Logger::log(LOG_INFO, "Successfully removed cgroup v2 directory: %s", cgroupDirectory.c_str());
        } else {
            if (errno != ENOENT) {
                Logger::log(LOG_DEBUG, "Failed to remove cgroup v2 directory %s: %s (errno=%d)",
                        cgroupDirectory.c_str(), strerror(errno), errno);
            }
        }
        return;
    }
    // List of controller subdirectories to remove (legacy layout)
    const char* controllers[] = {
        "cpu,cpuacct",
        "memory",
        "net_cls,net_prio",
        "pids"
    };
    bool anyRemoved = false;
    bool anyFailed = false;
    for (int i = 0; i < 4; i++) {
        string controllerPath = cgroupDirectory + controllers[i];
        DIR* dir = opendir(controllerPath.c_str());
        if (dir == NULL) {
            continue;
        }
        closedir(dir);
        Logger::log(LOG_DEBUG, "Removing cgroup directory: %s", controllerPath.c_str());
        if (rmdir(controllerPath.c_str()) == 0) {
            Logger::log(LOG_INFO, "Successfully removed cgroup directory: %s", controllerPath.c_str());
            anyRemoved = true;
        } else {
            Logger::log(LOG_WARNING, "Failed to remove cgroup directory %s: %s (errno=%d)",
                    controllerPath.c_str(), strerror(errno), errno);
            anyFailed = true;
        }
    }
    if (anyRemoved) {
        Logger::log(LOG_INFO, "Cgroup removal completed for: %s", cgroupDirectory.c_str());
    }
    if (anyFailed) {
        Logger::log(LOG_WARNING, "Some cgroup directories could not be removed. "
                "This may be because processes are still in the cgroup or permission issues.");
    }
}
