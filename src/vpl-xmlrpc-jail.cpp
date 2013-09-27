/**
 * version:		$Id: vpl-xmlrpc-jail.cpp,v 1.3 2011-04-07 14:46:16 juanca Exp $
 * package:		Part of vpl-xmlrpc-jail
 * copyright:	Copyright (C) 2009 Juan Carlos Rodríguez-del-Pino. All rights reserved.
 * license:		GNU/GPL, see LICENSE.txt or http://www.gnu.org/licenses/gpl-2.0.html
 **/

/**
 * Deamon vpl-xmlrpc-jail. jail for vpl using xmlrpc
 **/

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include "jail.h"
#include "util.h"

using namespace std;

/**
 * main accept command line [-d level] [-uri URI]
 * where level is the syslog log level and URI is the xmlrpc server uri
 */
int main(int const argc, const char ** const argv, char * const * const env) {
	openlog("vpl-xmlrpc-jail",LOG_PID,LOG_USER);
	//Set log level from command arg "-d level"
	string debugLevel=Util::getCommand(argc,argv,"-d");
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
	try{
		Jail jail(argc,argv,env);
		jail.process();
	}
	catch(const char * const me){
		syslog(LOG_ERR,"%s",me);
	}
	catch(...){
		syslog(LOG_ERR,"unexpected exception");
	}
	return 0;
}
