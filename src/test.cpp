/**
 * package:		Part of vpl-jail-system
 * copyright:	Copyright (C) 2014 Juan Carlos Rodríguez-del-Pino. All rights reserved.
 * license:		GNU/GPL, see LICENSE.txt or http://www.gnu.org/licenses/gpl-3.0.html
 **/
#include <cassert>
#include <iostream>
#include <exception>
#include "util.h"
#include "cgroup.h"
#include "configuration.h"
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
	assert(Base64::decode("SGVsbG8=")=="Hello"||(cerr <<Base64::decode("SGVsbG8=")<<endl,0));
	assert(Base64::decode("YWJjZGU=")=="abcde"||(cerr <<Base64::decode("YWJjZGU=")<<endl,0));
	assert(Base64::decode("w7HDocOR")=="ñáÑ"||(cerr <<Base64::decode("w7HDocOR")<<endl,0));
	assert(Base64::decode("w7HDocORPQ==")=="ñáÑ="||(cerr <<Base64::decode("w7HDocORPQ==")<<endl,0));
	assert(Base64::decode(Base64::encode("RFB 003.008\n"))=="RFB 003.008\n");
	string test(256,'\0');
	unsigned char *buf=(unsigned char *)test.data();
	for(int i=0; i<256;i++)
		buf[i]=i;
	string enc=Base64::encode(test);
	string dec=Base64::decode(enc);
	assert(dec.size()==256);
	unsigned char *raw= (unsigned char *)dec.data();
	for(int i=0; i<256;i++)
		assert(i==raw[i]);

}
void testRandom(){

}
void testTrim(){

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

void testSetCgroupFileSystem(){
	Cgroup::setBaseCgroupFileSystem("/sys/fs/cgroup");
	assert(Cgroup::getBaseCgroupFileSystem() == "/sys/fs/cgroup");
	assert(Cgroup::getBaseCgroupFileSystem() != "/sys/fs");
	Cgroup::setBaseCgroupFileSystem("/");
	assert(Cgroup::getBaseCgroupFileSystem() == "/");
	assert(Cgroup::getBaseCgroupFileSystem() != "//");
	Cgroup::setBaseCgroupFileSystem(" ");
	assert(Cgroup::getBaseCgroupFileSystem() == " ");
}

void testGetCPUAcctStat(){
	char buff[FILENAME_MAX];
	assert(getcwd(buff, FILENAME_MAX) != NULL);
	string currentWorkingDir(buff);
	Cgroup::setBaseCgroupFileSystem(currentWorkingDir);
	Cgroup cgroup("cgroup");
	map<string, int> result = cgroup.getCPUAcctStat();
	assert(result.find("user")->second == 36509);
	assert(result.find("system")->second == 3764);
}

void testGetCPUStat(){
	char buff[FILENAME_MAX];
	assert(getcwd(buff, FILENAME_MAX) != NULL);
	string currentWorkingDir(buff);
	Cgroup::setBaseCgroupFileSystem(currentWorkingDir);
	Cgroup cgroup("cgroup");
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
	Cgroup cgroup("cgroup");
	map<string, long int> result = cgroup.getMemoryStat();
	assert(result.find("cache")->second == 1626644480);
	assert(result.find("shmem")->second == 26406912);
	assert(result.find("mapped_file")->second == 351842304);
	assert(result.find("pgfault")->second == 2448732);
	assert(result.find("hierarchical_memory_limit")->second == 9223372036854771712);
}

void testGetCPUUsage(){
	char buff[FILENAME_MAX];
	assert(getcwd(buff, FILENAME_MAX) != NULL);
	string currentWorkingDir(buff);
	Cgroup::setBaseCgroupFileSystem(currentWorkingDir);
	Cgroup cgroup("cgroup");
	long int result = cgroup.getCPUUsage();
	assert(result == 406582887060);
}

void testGetNotify(){
	char buff[FILENAME_MAX];
	assert(getcwd(buff, FILENAME_MAX) != NULL);
	string currentWorkingDir(buff);
	Cgroup::setBaseCgroupFileSystem(currentWorkingDir);
	Cgroup cgroup("cgroup");
	int result = cgroup.getNotify();
	assert(result == 0);
}

void testGetReleaseAgent(){
	char buff[FILENAME_MAX];
	assert(getcwd(buff, FILENAME_MAX) != NULL);
	string currentWorkingDir(buff);
	Cgroup::setBaseCgroupFileSystem(currentWorkingDir);
	Cgroup cgroup("cgroup");
	string result = cgroup.getReleaseAgent();
	assert(result == "0");
}

void testGetCPUTasks(){
	char buff[FILENAME_MAX];
	assert(getcwd(buff, FILENAME_MAX) != NULL);
	string currentWorkingDir(buff);
	Cgroup::setBaseCgroupFileSystem(currentWorkingDir);
	Cgroup cgroup("cgroup");
	string tasks = cgroup.getCPUTasks();
	assert(tasks.find("6") != string::npos);
	assert(tasks.find("2172") != string::npos);
	assert(tasks.find("8") != string::npos);
	assert(tasks.find("4735") != string::npos);
}

void testGetNetPrioID(){
	char buff[FILENAME_MAX];
	assert(getcwd(buff, FILENAME_MAX) != NULL);
	string currentWorkingDir(buff);
	Cgroup::setBaseCgroupFileSystem(currentWorkingDir);
	Cgroup cgroup("cgroup");
	int prioId = cgroup.getNetPrioID();
	assert(prioId == 1);
}

void testGetPIDs(){
	char buff[FILENAME_MAX];
	assert(getcwd(buff, FILENAME_MAX) != NULL);
	string currentWorkingDir(buff);
	Cgroup::setBaseCgroupFileSystem(currentWorkingDir);
	Cgroup cgroup("cgroup");
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
	Cgroup cgroup("cgroup");
	map<string, int> prioMap = cgroup.getNetPrioMap();
	assert(prioMap.find("lo")->second == 0);
}

void testGetMemoryTasks(){
	char buff[FILENAME_MAX];
	assert(getcwd(buff, FILENAME_MAX) != NULL);
	string currentWorkingDir(buff);
	Cgroup::setBaseCgroupFileSystem(currentWorkingDir);
	Cgroup cgroup("cgroup");
	vector<int> tasks = cgroup.getMemoryTasks();
	assert(count(tasks.begin(), tasks.end(), 4745));
	assert(count(tasks.begin(), tasks.end(), 4743));
	assert(count(tasks.begin(), tasks.end(), 1));
	assert(count(tasks.begin(), tasks.end(), 2));
}

void testGetMemoryLimitInBytes(){
	char buff[FILENAME_MAX];
	assert(getcwd(buff, FILENAME_MAX) != NULL);
	string currentWorkingDir(buff);
	Cgroup::setBaseCgroupFileSystem(currentWorkingDir);
	Cgroup cgroup("cgroup");
	long int limit = cgroup.getMemoryLimitInBytes();
	assert(limit == 9223372036854771712);
}

void testGetMemoryUsageInBytes(){
	char buff[FILENAME_MAX];
	assert(getcwd(buff, FILENAME_MAX) != NULL);
	string currentWorkingDir(buff);
	Cgroup::setBaseCgroupFileSystem(currentWorkingDir);
	Cgroup cgroup("cgroup");
	long int usage = cgroup.getMemoryUsageInBytes();
	assert(usage == 3502428160);
}

void testGetMemNotify(){
	char buff[FILENAME_MAX];
	assert(getcwd(buff, FILENAME_MAX) != NULL);
	string currentWorkingDir(buff);
	Cgroup::setBaseCgroupFileSystem(currentWorkingDir);
	Cgroup cgroup("cgroup");
	int notify = cgroup.getMemNotify();
	assert(notify == 0);
}

void testGetMemReleaseAgent(){
	char buff[FILENAME_MAX];
	assert(getcwd(buff, FILENAME_MAX) != NULL);
	string currentWorkingDir(buff);
	Cgroup::setBaseCgroupFileSystem(currentWorkingDir);
	Cgroup cgroup("cgroup");
	int releaseAgent = Util::atoi(cgroup.getReleaseAgent());
	assert(releaseAgent == 0);
}

void testGetMemoryOOMControl(){
	char buff[FILENAME_MAX];
	assert(getcwd(buff, FILENAME_MAX) != NULL);
	string currentWorkingDir(buff);
	Cgroup::setBaseCgroupFileSystem(currentWorkingDir);
	Cgroup cgroup("cgroup");
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
	Cgroup cgroup("cgroup");
	cgroup.setNetPrioMap("eth0 1");
	map<string, int> prioMap = cgroup.getNetPrioMap();
	assert(prioMap.find("eth0")->second == 1);
}
void testSetCPUCloneChildren(){
	char buff[FILENAME_MAX];
	assert(getcwd(buff, FILENAME_MAX) != NULL);
	string currentWorkingDir(buff);
	Cgroup::setBaseCgroupFileSystem(currentWorkingDir);
	Cgroup cgroup("cgroup");
}
void testSetCPUProcs(){
	char buff[FILENAME_MAX];
		assert(getcwd(buff, FILENAME_MAX) != NULL);
		string currentWorkingDir(buff);
		Cgroup::setBaseCgroupFileSystem(currentWorkingDir);
		Cgroup cgroup("cgroup");
}
void testSetCPUNotify(){
	char buff[FILENAME_MAX];
	assert(getcwd(buff, FILENAME_MAX) != NULL);
	string currentWorkingDir(buff);
	Cgroup::setBaseCgroupFileSystem(currentWorkingDir);
	Cgroup cgroup("cgroup");
}
void testSetCPUReleaseAgentPath(){
	char buff[FILENAME_MAX];
	assert(getcwd(buff, FILENAME_MAX) != NULL);
	string currentWorkingDir(buff);
	Cgroup::setBaseCgroupFileSystem(currentWorkingDir);
	Cgroup cgroup("cgroup");
}
void testSetCPUs(){
	char buff[FILENAME_MAX];
	assert(getcwd(buff, FILENAME_MAX) != NULL);
	string currentWorkingDir(buff);
	Cgroup::setBaseCgroupFileSystem(currentWorkingDir);
	Cgroup cgroup("cgroup");
}

int main(){
	bool firstTime = true;
	while(true){
		try {
			//Test util
			testBase64Encode();
			testBase64Decode();
			testRandom();
			testTrim();
			testItos();
			testToUppercase();
			testCorrectFileName();
			//Test config
			//Test cgroup
			testSetCgroupFileSystem();
			testGetCPUAcctStat();
			testGetCPUStat();
			testGetMemoryStat();
			testGetCPUUsage();
			testGetNotify();
			testGetReleaseAgent();
			testGetCPUTasks();
			testGetNetPrioID();
			testGetPIDs();
			testGetNetPrioMap();
			testGetMemoryTasks();
			testGetMemoryLimitInBytes();
			testGetMemoryUsageInBytes();
			testGetMemNotify();
			testGetMemReleaseAgent();
			testGetMemoryOOMControl();
			testSetNetPrioMap();
			testSetCPUCloneChildren();
			testSetCPUProcs();
			testSetCPUNotify();
			testSetCPUReleaseAgentPath();
			testSetCPUs();
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



