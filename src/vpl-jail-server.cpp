/**
 * package:		Part of vpl-jail-system
 * copyright:	Copyright (C) 2009 Juan Carlos Rodríguez-del-Pino. All rights reserved.
 * license:		GNU/GPL, see LICENSE.txt or http://www.gnu.org/licenses/gpl-3.0.html
 **/

/**
 * Deamon vpl-jail-system. jail for vpl using xmlrpc
 **/
#include <exception>
#include "vpl-jail-server.h"

Daemon* Daemon::singlenton=NULL;
bool Daemon::finishRequest=false;
using namespace std;
void setLogLevel(string debugLevel){
	openlog("vpl-jail-system",LOG_PID,LOG_DAEMON);
	if(debugLevel.size()>0){
		int mlevels[8]={LOG_EMERG,  LOG_ALERT, LOG_CRIT, LOG_ERR, LOG_WARNING,
				LOG_NOTICE, LOG_INFO, LOG_DEBUG};
		int level=atoi(debugLevel.c_str());
		if(level>7 || level<0) level=7;
		setlogmask(LOG_UPTO(mlevels[level]));
		syslog(LOG_INFO,"Set log mask up to %s",debugLevel.c_str());
	}else{
		setlogmask(LOG_UPTO(LOG_ERR));
	}
}

/**
 * main accept command line [-d level] [-uri URI]
 * where level is the syslog log level and URI is the xmlrpc server uri
 */
int main(int const argc, const char ** const argv, char * const * const env) {
	//Set log level from command arg "-d level"
	setLogLevel(Util::getCommand(argc,argv,"-d"));
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
