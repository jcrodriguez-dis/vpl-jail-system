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
#include <fcntl.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>

using namespace std;

class Logger {
	static int loglevel;
	static bool foreground;
	static bool traceEnabled;
	static const char * const levelName[8];
	static string escapeLogField(const string &value) {
		string escaped;
		escaped.reserve(value.size());
		for (size_t i = 0; i < value.size(); i++) {
			unsigned char character = value[i];
			if (character < 0x20 || character == 0x7f || character == '|') {
				escaped += ' ';
			} else {
				escaped += (char)character;
			}
		}
		return escaped;
	}
public:
	static void setLogLevel(int level, bool foreground) {
		setForeground(foreground);
		Logger::traceEnabled = level >= 8;
		openlog("vpl-jail-system", LOG_PID, LOG_DAEMON);
		if (level > 7 || level < 0) {
			level = 7;
		}
		Logger::loglevel = level;
		log(LOG_INFO, "Set log mask up to LOG_%s", levelName[level]);
	}
	static int getLogLevel() {
		return Logger::loglevel;
	}
	static bool isTraceEnabled() {
		return Logger::traceEnabled;
	}
	static void setForeground(bool foreground) {
		Logger::foreground = foreground;
	}
	static bool isForeground() {
		return Logger::foreground;
	}
	static void logRequest(const string clientIP, bool secure,
			const string request, const string extra="") {
		const char *logDir = "/var/log/vpl";
		const char *logFile = "/var/log/vpl/vpl-jail-server.log";
		if (mkdir(logDir, 0750) != 0 && errno != EEXIST) {
			log(LOG_ERR, "Cannot create request log directory %s: %m", logDir);
			return;
		}
		struct stat info;
		if (stat(logDir, &info) != 0 || !S_ISDIR(info.st_mode) ||
				info.st_uid != 0 || (info.st_mode & 022)) {
			log(LOG_ERR, "Insecure request log directory %s", logDir);
			return;
		}
		int fd = open(logFile, O_WRONLY | O_APPEND | O_CREAT | O_CLOEXEC | O_NOFOLLOW, 0640);
		if (fd < 0) {
			log(LOG_ERR, "Cannot open request log %s: %m", logFile);
			return;
		}
		time_t currentTime = time(NULL);
		struct tm localTime;
		char dateTime[32];
		localtime_r(&currentTime, &localTime);
		strftime(dateTime, sizeof(dateTime), "%Y-%m-%d %H:%M:%S", &localTime);
		string dateTimeStr = string(dateTime);
		string safeClientIP = " | " + escapeLogField(clientIP);
		string safeSecure = secure ? " | s" : " | n";
		string safeRequest = " | " + escapeLogField(request);
		string safeExtra = extra.empty() ? "" : " | " + escapeLogField(extra);
		string line = dateTimeStr + safeClientIP + safeSecure + safeRequest + safeExtra + "\n";
		if (write(fd, line.data(), line.size()) != (ssize_t)line.size()) {
			log(LOG_ERR, "Cannot write request log %s: %m", logFile);
		}
		close(fd);
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
		    time_t currentTime = time(NULL);
    		const char* formatString = "%Y-%m-%d %H:%M:%S";
		    char dateTime[80];
		    strftime(dateTime, sizeof(dateTime), formatString, std::localtime(&currentTime));
			clog << dateTime << " " << levelName[level] << ":" << getpid() << ": " << buf << endl;
		}
		syslog(level, "%s", buf);
	}
};

#endif
