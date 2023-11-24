/**
 * package:		Part of vpl-jail-system
 * copyright:	Copyright (C) 2021 Juan Carlos Rodríguez-del-Pino. All rights reserved.
 * license:		GNU/GPL, see LICENSE.txt or http://www.gnu.org/licenses/gpl-3.0.html
 **/

#include "basetest.h"
#include <cassert>
#include "../src/configuration.h"

class ConfigurationTestFull: public Configuration {
protected:
	ConfigurationTestFull(): Configuration("./configfiles/full.txt") {}
public:
	static Configuration* getConfiguration(){
		singlenton = new ConfigurationTestFull();
		return singlenton;
	}
};

class ConfigurationTestEmpty: public Configuration {
protected:
	ConfigurationTestEmpty(): Configuration("./configfiles/empty.txt") {}
public:
	static Configuration* getConfiguration(){
		singlenton = new ConfigurationTestEmpty();
		return singlenton;
	}
};

class ConfigurationTest: public BaseTest {
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
		assert(config->getSSLCipherSuites() == "");
		assert(config->getHSTSMaxAge() == -1);
		assert(config->getSSLCertFile() == "/etc/vpl/cert.pem");
		assert(config->getSSLKeyFile() == "/etc/vpl/key.pem");
		assert(config->getUseCGroup() == false);
		assert(config->getRequestMaxSize() == Util::memSizeToBytesi("128 Mb"));
		assert(config->getResultMaxSize() == Util::memSizeToBytesi("1 Mb"));
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
		assert(taskOnly.size() == 6);
		assert(taskOnly[0] == "10.10.3.");
		assert(taskOnly[1] == "193.22.144.23");
		assert(taskOnly[2] == "196.23.");
		assert(taskOnly[3] == "17.18.19.20");
		assert(taskOnly[4] == "21.22.23.24");
		assert(taskOnly[5] == "1.2.3.4");
		assert(config->getInterface() == "128.1.1.7");
		assert(config->getURLPath() == "/secr$et$");
		assert(config->getPort() == 8081);
		assert(config->getSecurePort() == 44300);
		assert(config->getLogLevel() == 1);
		assert(config->getFail2Ban() == 7);
		assert(config->getSSLCipherList() == "EDFG");
		assert(config->getSSLCipherSuites() == "MYSUITE:OTHERSUITE");
		assert(config->getHSTSMaxAge() == 31536000);
		assert(config->getSSLCertFile() == "/etc/ssl/public/cert.pem");
		assert(config->getSSLKeyFile() == "/etc/ssl/private/key.pem");
		assert(config->getUseCGroup() == true);
		assert(config->getRequestMaxSize() == Util::memSizeToBytesi("333 Mb"));
		assert(config->getResultMaxSize() == Util::memSizeToBytesi("524 Mb"));
	}
public:
	string name() {
		return "Configuration class";
	}
	void launch() {
		testConfigurationEmpty();
		testConfigurationFull();
	}
};

ConfigurationTest configurationTest;
