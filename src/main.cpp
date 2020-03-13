/**
 * package:		Part of vpl-jail-system
 * copyright:	Copyright (C) 2009 Juan Carlos Rodríguez-del-Pino. All rights reserved.
 * license:		GNU/GPL, see LICENSE.txt or http://www.gnu.org/licenses/gpl-3.0.html
 **/

#include <exception>
#include "vpl-jail-server.h"
#include "configuration.h"

using namespace std;
void setLogLevel(int level){
	openlog("vpl-jail-system",LOG_PID,LOG_DAEMON);
	if(level>7 || level<0) level=7;
	int mlevels[8]={LOG_EMERG, LOG_ALERT, LOG_CRIT, LOG_ERR, LOG_WARNING,
			LOG_NOTICE, LOG_INFO, LOG_DEBUG};

	setlogmask(LOG_UPTO(mlevels[level]));
	syslog(LOG_INFO,"Set log mask up to %d", level);
}

/**
 * main accept command line [-d level] [-uri URI]
 * where level is the syslog log level and URI is the xmlrpc server uri
 */
int main(int const argc, const char ** const argv, char * const * const env) {
	setLogLevel(3);
	Configuration *conf = Configuration::getConfiguration();
	setLogLevel(conf->getLogLevel());
	if ( conf->getLogLevel() >= LOG_INFO) {
		conf->readConfigFile(); // Reread configuration file to show values in log
	}
	int exitStatus=static_cast<int>(internalError);
	try{
		Daemon::getDaemon()->loop();
		exitStatus=EXIT_SUCCESS;
	}
	catch(HttpException &exception){
		syslog(LOG_ERR,"%s",exception.getLog().c_str());
		exitStatus=static_cast<int>(httpError);
	}
	catch(const string &me){
		syslog(LOG_ERR,"%s",me.c_str());
	}
	catch(const char * const me){
		syslog(LOG_ERR,"%s",me);
	}
	catch(std::exception &e){
		syslog(LOG_ERR,"unexpected exception: %s %s:%d",e.what(),__FILE__,__LINE__);
	}
	catch(...){
		syslog(LOG_ERR,"unexpected exception %s:%d",__FILE__,__LINE__);
	}
	exit(exitStatus);
}
