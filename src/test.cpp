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
	Cgroup cgroup("test");
	Cgroup::setBaseCgroupFileSystem("/sys/fs");
	map<string, int> result = cgroup.getCPUAcctStat();
	assert(result.count("user"));
	assert(result.count("system"));
	assert(!result.count(" "));
	assert(!result.count("usersystem"));
	assert(!result.count("user "));
}

void testGetCPUStat(){
	Cgroup cgroup("cgroup");
	map<string, int> result = cgroup.getCPUStat();
	assert(result.count("nr_throttled"));
	assert(result.count("nr_periods"));
	assert(result.count("throttled_time"));
	assert(!result.count("throttled_time "));
	assert(!result.count(" nr_periods"));
}

void testGetMemoryStat(){
	Cgroup cgroup("cgroup");
	map<string, int> result = cgroup.getCPUStat();
	assert(result.count("cache"));
	assert(result.count("shmem"));
	assert(result.count("mapped_file"));
	assert(result.count("pgfault"));
	assert(result.count("hierarchical_memory_limit"));
}

void testSetMemHardwall(){
	Cgroup cgroup("cgroup");
	cgroup.setMemHardwall(true);
	string cgroupDirectory = Cgroup::getBaseCgroupFileSystem();
	assert(cgroup.getMemHardwall() == true);
	assert(cgroup.getMemHardwall() != false);
	assert(cgroup.getMemHardwall() != true);
	assert(cgroup.getMemHardwall() == false);
	assert(cgroup.getMemHardwall() != true);
}

void testGetCPUUsage(){

}

void testGetNotify(){

}

void testGetReleaseAgent(){

}

void testGetCPUTasks(){

}

void testGetNetPrioID(){

}

void testGetPIDs(){

}

void testGetNetPrioMap(){

}

void testGetCloneChildren(){

}

void testGetMemoryMigrate(){

}

void testGetMemoryLimitInBytes(){

}




int main(){
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
		testSetMemHardwall();
	} catch (exception &e) {
		cerr << e.what() << endl;
		return 1;
	}
	return 0;
}



