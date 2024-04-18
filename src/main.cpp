/**
 * package:		Part of vpl-jail-system
 * copyright:	Copyright (C) 2009 Juan Carlos Rodríguez-del-Pino. All rights reserved.
 * license:		GNU/GPL, see LICENSE.txt or http://www.gnu.org/licenses/gpl-3.0.html
 **/

#include <exception>
#include "vpl-jail-server.h"
#include "configuration.h"
#include "log.h"

using namespace std;

#ifndef TEST
/**
 * main accept command line "foreground" to go non-daemon run.
 */
int main(int const argc, const char ** const argv, char * const * const env) {
	bool foreground = false;
	bool inContaier = false;
	for (int i = 1; i < argc; i++) {
		string arg = argv[i];
		if (arg == "foreground") {
			foreground = true;
		}
		if (arg ==  "in_container") {
			inContaier = true;
		}
		if (arg ==  "version" || arg == "-version") {
			cout << Util::version() <<endl;
			exit(EXIT_SUCCESS);
		}
	}
	cout << "Server running";
	if (foreground) cout << " in foreground mode (non-daemon mode)";
	if (inContaier) cout << " inside a container (no chroot used)";
	cout << endl;
	Logger::setLogLevel(LOG_ERR, foreground);
	Configuration *conf = Configuration::getConfiguration();
	Logger::setLogLevel(conf->getLogLevel(), foreground);
	if ( conf->getLogLevel() >= LOG_INFO) {
		conf->readConfigFile(); // Reread configuration file to show values in log
	}
	if (conf->getJailPath() == "" && ! inContaier) {
		Logger::log(LOG_EMERG, "Jail directory root \"/\" but not running in container");
		exit(1);
	}
	if (conf->getJailPath() != "" && inContaier) {
		Logger::log(LOG_EMERG, "Running in container but Jail directory not root \"/\"");
		exit(1);
	}
	int exitStatus=static_cast<int>(internalError);
	try{
		Daemon* runner = Daemon::getRunner();
		if (foreground) {
			runner->foreground();
		} else {
			runner->daemonize();
		}
		Logger::log(LOG_INFO, "VPL Jail Server %s started", Util::version());
		runner->loop();
		exitStatus = EXIT_SUCCESS;
	}
	catch(HttpException &exception) {
		Logger::log(LOG_WARNING, "%s", exception.getLog().c_str());
		exitStatus=static_cast<int>(httpError);
	}
	catch(const string &me) {
		exitStatus = EXIT_FAILURE;
		Logger::log(LOG_WARNING, "%s", me.c_str());
	}
	catch(const char * const me) {
		exitStatus = EXIT_FAILURE;
		Logger::log(LOG_WARNING, "%s",me);
	}
	catch(std::exception &e) {
		exitStatus = EXIT_FAILURE;
		Logger::log(LOG_EMERG, "Unexpected exception: %s %s:%d", e.what(), __FILE__, __LINE__);
	}
	catch(...){
		exitStatus = EXIT_FAILURE;
		Logger::log(LOG_EMERG, "Unexpected exception %s:%d", __FILE__, __LINE__);
	}
	exit(exitStatus);
}
#endif
