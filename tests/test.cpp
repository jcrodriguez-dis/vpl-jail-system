/**
 * package:		Part of vpl-jail-system
 * copyright:	Copyright (C) 2014 Juan Carlos Rodríguez-del-Pino. All rights reserved.
 * license:		GNU/GPL, see LICENSE.txt or http://www.gnu.org/licenses/gpl-3.0.html
 **/
#include <cassert>
#include <iostream>
#include <algorithm>
#include <exception>
#include "../src/util.h"
#include "../src/cgroup.h"
#include "../src/configuration.h"
#include <syslog.h>
#include <stdio.h>

using namespace std;

void testBase64Encode(){
	assert(Base64::encode("Hello")=="SGVsbG8="||(cerr <<Base64::encode("Hello")<<endl,0));
	assert(Base64::encode("abcde")=="YWJjZGU="||(cerr <<Base64::encode("abcde")<<endl,0));
	assert(Base64::encode("ñáÑ")=="w7HDocOR"||(cerr <<Base64::encode("ñáÑ")<<endl,0));
	assert(Base64::encode("ñáÑ=")=="w7HDocORPQ=="||(cerr <<Base64::encode("ñáÑ=")<<endl,0));
}
void testBase64Decode(){
	const int ibyte = 256;
	assert(Base64::decode("SGVsbG8=")=="Hello"||(cerr <<Base64::decode("SGVsbG8=")<<endl,0));
	assert(Base64::decode("YWJjZGU=")=="abcde"||(cerr <<Base64::decode("YWJjZGU=")<<endl,0));
	assert(Base64::decode("w7HDocOR")=="ñáÑ"||(cerr <<Base64::decode("w7HDocOR")<<endl,0));
	assert(Base64::decode("w7HDocORPQ==")=="ñáÑ="||(cerr <<Base64::decode("w7HDocORPQ==")<<endl,0));
	assert(Base64::decode(Base64::encode("RFB 003.008\n"))=="RFB 003.008\n");
	string test(ibyte, '\0');
	unsigned char *buf = (unsigned char *)test.data();
	for(int i = 0; i < ibyte;i++)
		buf[i]=i;
	string enc=Base64::encode(test);
	string dec=Base64::decode(enc);
	assert(dec.size() == ibyte);
	unsigned char *raw = (unsigned char *)dec.data();
	for(int i=0; i < ibyte;i++)
		assert(i == raw[i]);

}

class ConfigurationTestFull: public Configuration {
protected:
	ConfigurationTestFull(): Configuration("./configfiles/full.txt") {
}
public:
	static Configuration* getConfiguration(){
		singlenton = new ConfigurationTestFull();
		return singlenton;
	}
};

class ConfigurationTestEmpty: public Configuration {
protected:
	ConfigurationTestEmpty(): Configuration("./configfiles/empty.txt") {
}
public:
	static Configuration* getConfiguration(){
		singlenton = new ConfigurationTestEmpty();
		return singlenton;
	}
};

void testConfigurationEmpty() {
	Configuration *config = ConfigurationTestEmpty::getConfiguration();
	ExecutionLimits limits = config->getLimits();
	assert(limits.maxfilesize == Util::memSizeToBytesi("64 mb"));
	assert(limits.maxmemory == Util::memSizeToBytesi("2000 mb"));
	assert(limits.maxprocesses == 500);
	assert(limits.maxtime == 1200);
	assert(config->getJailPath() == "/jail");
	assert(config->getControlPath() == "/var/vpl-jail-system");
	assert(config->getMinPrisoner() == 10000);
	assert(config->getMaxPrisoner() == 20000);
	vector<string> taskOnly = config->getTaskOnlyFrom();
	assert(taskOnly.size() == 0);
	assert(config->getInterface() == "");
	assert(config->getURLPath() == "/");
	assert(config->getPort() == 80);
	assert(config->getSecurePort() == 443);
	assert(config->getLogLevel() == 0);
	assert(config->getFail2Ban() == 0);
	assert(config->getSSLCipherList() == "");
	assert(config->getSSLCertFile() == "/etc/vpl/cert.pem");
	assert(config->getSSLKeyFile() == "/etc/vpl/key.pem");
	assert(config->getUseCGroup() == false);
	assert(config->getRequestMaxSize() == Util::memSizeToBytesi("128 mb"));
	assert(config->getResultMaxSize() == Util::memSizeToBytesi("32 Kb"));
}

void testConfigurationFull() {
	Configuration *config = ConfigurationTestFull::getConfiguration();
	ExecutionLimits limits = config->getLimits();
	assert(limits.maxfilesize == Util::memSizeToBytesi("65 mb"));
	assert(limits.maxmemory == Util::memSizeToBytesi("1998 mb"));
	assert(limits.maxprocesses == 500);
	assert(limits.maxtime == 88888);
	assert(config->getJailPath() == "/rejail/algo");
	assert(config->getControlPath() == "/mnt/opt/var/vpl-jail-system");
	assert(config->getMinPrisoner() == 11111);
	assert(config->getMaxPrisoner() == 11222);
	vector<string> taskOnly = config->getTaskOnlyFrom();
	assert(taskOnly.size() == 3);
	assert(taskOnly[0] == "10.10.3.");
	assert(taskOnly[1] == "193.22.144.23");
	assert(taskOnly[2] == "196.23.");
	assert(config->getInterface() == "128.1.1.7");
	assert(config->getURLPath() == "/secr$et$");
	assert(config->getPort() == 8081);
	assert(config->getSecurePort() == 44300);
	assert(config->getLogLevel() == 1);
	assert(config->getFail2Ban() == 7);
	assert(config->getSSLCipherList() == "EDFG");
	assert(config->getSSLCertFile() == "/etc/ssl/public/cert.pem");
	assert(config->getSSLKeyFile() == "/etc/ssl/private/key.pem");
	assert(config->getUseCGroup() == true);
	assert(config->getRequestMaxSize() == Util::memSizeToBytesi("333 Mb"));
	assert(config->getResultMaxSize() == Util::memSizeToBytesi("524 Mb"));
}

void testTrim(){
    string val = ""; Util::trimAndRemoveQuotes(val); assert(val == "");
    val = "algo"; Util::trimAndRemoveQuotes(val); assert(val == "algo");
    val = "algo más"; Util::trimAndRemoveQuotes(val); assert(val == "algo más");
    val = "hola que     tal nada"; Util::trimAndRemoveQuotes(val); assert(val == "hola que     tal nada");
    val = "      "; Util::trimAndRemoveQuotes(val); assert(val == "");
    val = "  a  b "; Util::trimAndRemoveQuotes(val); assert(val == "a  b");
    val = "           a   ff  b    "; Util::trimAndRemoveQuotes(val); assert(val == "a   ff  b");
    val = "''"; Util::trimAndRemoveQuotes(val); assert(val == "");
    val = "\"\""; Util::trimAndRemoveQuotes(val); assert(val == "");
    val = "'  a  b '"; Util::trimAndRemoveQuotes(val); assert(val == "  a  b ");
    val = "\"  a  b \""; Util::trimAndRemoveQuotes(val); assert(val == "  a  b ");
    val = "   '  a  b '"; Util::trimAndRemoveQuotes(val); assert(val == "  a  b ");
    val = "       \"  a  b \"        "; Util::trimAndRemoveQuotes(val); assert(val == "  a  b ");
}

void testItos(){
	assert(Util::itos(0) == "0");
	assert(Util::itos(3) == "3");
	assert(Util::itos(-32780) == "-32780");
	assert(Util::itos(2038911111) == "2038911111");
	assert(Util::itos(-2038911111) == "-2038911111");
}

void testToUppercase(){
	assert(Util::toUppercase("") == "");
	assert(Util::toUppercase("a") == "A");
	assert(Util::toUppercase("a") == "A");
	assert(Util::toUppercase("á") == "á");
	assert(Util::toUppercase("ábCÑ()999  {ç") == "áBCÑ()999  {ç");
	assert(Util::toUppercase("jc\ndis") == "JC\nDIS");
	assert(Util::toUppercase("jc\\ndis") == "JC\\NDIS");
}

void testCorrectFileName(){
	assert(!Util::correctFileName("jc\ndis"));
	assert(!Util::correctFileName("jc\005dis"));
	assert(!Util::correctFileName("jc1di*s"));
	assert(!Util::correctFileName("jc1dis!"));
	assert(!Util::correctFileName("mal,c"));
	assert(!Util::correctFileName("mal:c"));
	assert(!Util::correctFileName("klsdjaf?"));
	assert(!Util::correctFileName("kl@sdjaf"));
	assert(!Util::correctFileName("kl'sdjaf"));
	assert(!Util::correctFileName("kl[sdjaf"));
	assert(!Util::correctFileName("klsdjaf^"));
	assert(!Util::correctFileName("kl\"sdjaf"));
	assert(!Util::correctFileName("klsdjaf\x60"));
	assert(!Util::correctFileName("(klsdjaf"));
	assert(!Util::correctFileName("klsd{jaf"));
	assert(!Util::correctFileName("klsdja~f"));
	assert(!Util::correctFileName("kls/djaf"));
	assert(!Util::correctFileName("\\klsdjaf.pp"));
	assert(!Util::correctFileName("nose..mal"));
	assert(!Util::correctFileName(""));
	assert(!Util::correctFileName(
			"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
			"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
			"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
			"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"));
	assert(Util::correctFileName("a"));
	assert(Util::correctFileName("kkkk"));
	assert(Util::correctFileName("a.raro"));
	assert(Util::correctFileName("normal.cvs"));
	assert(Util::correctFileName("normal.cvs.old"));
	assert(Util::correctFileName("normal-con-guiones.cvs"));
	assert(Util::correctFileName("normal_con_guiones.cvs"));
	assert(Util::correctFileName("b"));
	assert(Util::correctFileName("fichero con espacios. y varios . puntos"));
}

void testCleanPATH(){
	assert(Configuration::generateCleanPATH("","")=="");
	assert(Configuration::generateCleanPATH("/usr","")=="");
	assert(Configuration::generateCleanPATH("","/usr:/usr/bin")=="/usr:/usr/bin");
	assert(Configuration::generateCleanPATH("/usr","/bin")=="/bin");
	assert(Configuration::generateCleanPATH("/usr","/bin:/kk:/sbin")=="/bin:/sbin");
	assert(Configuration::generateCleanPATH("/usr","/bin:/kk:/sbin:/local/bin:/local/nada")=="/bin:/sbin:/local/bin");
}

void testTimeOfFileModification(){
	string fileName = "timeOfFileModification.test_file";
	time_t now = time(NULL);
	Util::writeFile(fileName, fileName, geteuid());
	time_t modification = Util::timeOfFileModification(fileName);
	Util::deleteFile(fileName);
	assert(now <= modification &&  (modification - now) < 3 );
}

void testWriteReadRemoveFile(){
	Util::writeFile("/tmp/to_be_or_not_to_be", "mi texto único", getuid());
	assert( Util::fileExists("/tmp/to_be_or_not_to_be") );
	assert( Util::readFile("/tmp/to_be_or_not_to_be") == "mi texto único" );
	Util::deleteFile("/tmp/to_be_or_not_to_be");
	assert( ! Util::fileExists("/tmp/to_be_or_not_to_be") );
}

void testMemSizeToBytesl() {
	int long l1234 = 1234;
	int long kb1219 = 1219 * 1024;
	int long mb435 = 435 * 1024 * 1024;
	int long gb8 = 8l * 1024 * 1024 * 1024;
	assert(Util::memSizeToBytesl("nada") == 0);
	assert(Util::memSizeToBytesl("-10") == 0);
	assert(Util::memSizeToBytesl("0") == 0);
	assert(Util::memSizeToBytesl("0 Mb") == 0);
	assert(Util::memSizeToBytesl("1234") == l1234);
	assert(Util::memSizeToBytesl("1219 kbytes") == kb1219);
	assert(Util::memSizeToBytesl("1219K") == kb1219);
	assert(Util::memSizeToBytesl("435 Mbytes") == mb435);
	assert(Util::memSizeToBytesl("435m") == mb435);
	assert(Util::memSizeToBytesl("8gbytes") == gb8);
	assert(Util::memSizeToBytesl("8     G") == gb8);
}

void testMemSizeToBytesi() {
	int l1234 = 1234;
	int kb1219 = 1219 * 1024;
	int mb435 = 435 * 1024 * 1024;
	int gb1 = 1024 * 1024 * 1024;
	assert(Util::memSizeToBytesi("nada") == 0);
	assert(Util::memSizeToBytesi("-10") == 0);
	assert(Util::memSizeToBytesi("0") == 0);
	assert(Util::memSizeToBytesi("0 Mb") == 0);
	assert(Util::memSizeToBytesi("1234") == l1234);
	assert(Util::memSizeToBytesi("1219 kbytes") == kb1219);
	assert(Util::memSizeToBytesi("1219K") == kb1219);
	assert(Util::memSizeToBytesi("435 Mbytes") == mb435);
	assert(Util::memSizeToBytesi("435m") == mb435);
	assert(Util::memSizeToBytesi("1gbytes") == gb1);
	assert(Util::memSizeToBytesi("1     G") == gb1);
}
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

int main(){
	bool firstTime = true;
	while(true){
		try {
			//Test util
			testBase64Encode();
			testBase64Decode();
			testTrim();
			testItos();
			testToUppercase();
			testCorrectFileName();
			testWriteReadRemoveFile();
			testTimeOfFileModification();
			testMemSizeToBytesl();
			testMemSizeToBytesi();
			testConfigurationEmpty();
			testConfigurationFull();
			//Test cgroup
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
			cout << "Test Finish" << endl;
		} catch (exception &e) {
			cerr << e.what() << endl;
			if (firstTime){
				openlog("vpl-jail-system-test",LOG_PID,LOG_DAEMON);
				setlogmask(LOG_UPTO(LOG_INFO));
				firstTime = false;
				continue;
			}
			return 1;
		} catch (HttpException &e) {
			cerr << e.getMessage() << endl;
			if (firstTime){
				openlog("vpl-jail-system-test",LOG_PID,LOG_DAEMON);
				setlogmask(LOG_UPTO(LOG_INFO));
				firstTime = false;
				continue;
			}
			return 2;
		}
		break;
	}
	return 0;
}



