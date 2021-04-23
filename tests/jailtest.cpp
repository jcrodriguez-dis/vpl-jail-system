/**
 * package:		Part of vpl-jail-system
 * copyright:	Copyright (C) 2021 Juan Carlos Rodríguez-del-Pino. All rights reserved.
 * license:		GNU/GPL, see LICENSE.txt or http://www.gnu.org/licenses/gpl-3.0.html
 **/

#include "basetest.h"
#include <cassert>
#include "../src/jail.h"

class ConfigurationTestJail: public Configuration {
protected:
	ConfigurationTestJail(): Configuration("./configfiles/full.txt") {}
public:
	static Configuration* getConfiguration(){
		singlenton = new ConfigurationTestJail();
		return singlenton;
	}
};
class JailTest: public BaseTest {
	void testisValidIPforRequest() {
		Configuration *config = ConfigurationTestJail::getConfiguration();
		// TASK_ONLY_FROM=10.10.3. 193.22.144.23 196.23.
		{
			Jail jail("10.10.3.1");
			assert(jail.isValidIPforRequest());
		}
		{
			Jail jail("193.22.144.23");
			assert(jail.isValidIPforRequest());
		}
		{
			Jail jail("196.23.144.23");
			assert(jail.isValidIPforRequest());
		}
		{
			Jail jail("195.22.144.23");
			assert(! jail.isValidIPforRequest());
		}
		{
			Jail jail("196.22.144.23");
			assert(! jail.isValidIPforRequest());
		}
		{
			Jail jail("196.23.254.254");
			assert(jail.isValidIPforRequest());
		}
	}
	
	void testisRequestingCookie() {
		Configuration *config = ConfigurationTestJail::getConfiguration();
		Jail jail("1.1.1.1");
		string ticket;
		assert( jail.isRequestingCookie("/97412394712/httpPassthrough", ticket) );
		assert( ticket == "97412394712" );
		assert( jail.isRequestingCookie("/970050412394712/httpPassthrough", ticket) );
		assert( ticket == "970050412394712" );
		assert( ! jail.isRequestingCookie("/97412394712/execution", ticket) );
		assert( ! jail.isRequestingCookie("/97412394712/monitor", ticket) );
	}

	void testpredefinedURLResponse() {
		Configuration *config = ConfigurationTestJail::getConfiguration();
		Jail jail("1.1.1.1");
		string ticket;
		assert( ! jail.predefinedURLResponse("/OK").empty() );
		assert( ! jail.predefinedURLResponse("/ok").empty() );
		assert( ! jail.predefinedURLResponse("/robots.txt").empty() );
		assert( ! jail.predefinedURLResponse("/favicon.ico").empty() );
		assert( jail.predefinedURLResponse("/index.html").empty() );
		assert( jail.predefinedURLResponse("/index.php").empty() );
	}

public:
	string name() {
		return "Jail class";
	}
	void launch() {
		testisValidIPforRequest();
		testisRequestingCookie();
		testpredefinedURLResponse();
	}
};

JailTest jailtest;
