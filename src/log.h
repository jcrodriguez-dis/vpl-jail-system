/**
 * package:		Part of vpl-jail-system
 * copyright:	Copyright (C) 2023 Juan Carlos Rodríguez-del-Pino. All rights reserved.
 * license:		GNU/GPL, see LICENSE.txt or http://www.gnu.org/licenses/gpl-3.0.html
 **/

#ifndef VPL_LOG_INC_H
#define VPL_LOG_INC_H
#if HAVE_CONFIG_H
#include <config.h>
#endif
#include <iostream>
#include <ctime>
#include <syslog.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>

using namespace std;

class Logger {
	static int loglevel;
	static bool foreground;
	static const char * const levelName[8];
public:
	static void setLogLevel(int level, bool foreground) {
		setForeground(foreground);
		openlog("vpl-jail-system", LOG_PID, LOG_DAEMON);
		if (level > 7 || level < 0) {
			level = 7;
		}
		Logger::loglevel = level;
		log(LOG_INFO, "Set log mask up to LOG_%s", levelName[level]);
	}
	static void setForeground(bool foreground) {
		Logger::foreground = foreground;
	}
	static bool isForeground() {
		return Logger::foreground;
	}
	static void log(int level, const char *format, ...) {
		const int buflen = 1000;
		if (level > 7 || level < 0) {
			level = 0;
		}
		if (level > Logger::loglevel) {
			return;
		}
		char buf[buflen];
		va_list args;
		va_start(args, format);
    	vsnprintf(buf, buflen, format, args);
    	va_end(args);		
		if (isForeground()) {
		    time_t currentTime = time(nullptr);
    		const char* formatString = "%Y-%m-%d %H:%M:%S";
		    char dateTime[80];
		    strftime(dateTime, sizeof(dateTime), formatString, std::localtime(&currentTime));
			clog << dateTime << " " << levelName[level] << ":" << getpid() << ": " << buf << endl;
		}
		syslog(level, "%s", buf);
	}
};

#endif
