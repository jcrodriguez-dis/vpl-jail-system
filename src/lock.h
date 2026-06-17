/**
 * @package:   Part of vpl-jail-system
 * @copyright: Copyright (C) 2014 Juan Carlos Rodríguez-del-Pino
 * @license:   GNU/GPL, see LICENSE.txt or http://www.gnu.org/licenses/gpl-3.0.html
 **/

#ifndef LOCK_H_
#define LOCK_H_
#include <string>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <syslog.h>

#include "util.h"
#include "configuration.h"

using namespace std;

class Lock{
	string filePath;
	static string getBaseDir() {
		Configuration* configuration = Configuration::getConfiguration();
		return configuration->getControlPath() + "/locks";
	}
	static string transformDirPath(string &dirPath) {
		string res = dirPath;
		for ( size_t i = 0; i < res.size(); i++ ) {
			if ( res.at(i) == '/' ) {
				res[i] = '+';
			}
		}
		return res;
	}
public:
	Lock(string DirPath){
		string baseDir = getBaseDir();
		if ( ! Util::dirExists(baseDir) ) {
			Util::createDir(baseDir, 0, baseDir.size());
		}
		filePath = baseDir + "/" + transformDirPath(DirPath);
		int ntry = 0;
		int fd;
		while ( (fd = open(filePath.c_str(), O_CREAT|O_EXCL|O_WRONLY, 0600)) == -1
				&& ntry < 250 ) {
			// Try to read the PID stored in the existing lock file
			int rfd = open(filePath.c_str(), O_RDONLY);
			if (rfd != -1) {
				char buf[32] = {};
				ssize_t n = read(rfd, buf, sizeof(buf) - 1);
				close(rfd);
				if (n > 0) {
					pid_t lockPid = (pid_t)atoi(buf);
					if (lockPid == getpid()) {
						// Re-entrant attempt by the same process — programming error
						Logger::log(LOG_WARNING, "Re-entrant lock attempt by pid %d for %s",
								lockPid, filePath.c_str());
					} else if (lockPid > 0 && !Util::processExists(lockPid)) {
						// Lock holder process is dead — remove stale lock immediately
						if (unlink(filePath.c_str()) == 0) {
							Logger::log(LOG_DEBUG, "Removed stale lock from dead pid %d: %s",
									lockPid, filePath.c_str());
							continue;
						}
					}
				}
			}
			Util::sleep(20000);
			ntry++;
		}
		if (fd != -1) {
			// Store our PID so other processes can check our liveness
			char buf[32];
			int len = snprintf(buf, sizeof(buf), "%d", getpid());
			if(write(fd, buf, len) != len) {
				close(fd);
				unlink(filePath.c_str());
				throw std::runtime_error("Failed to write PID to lock file: " + filePath);
			}
			close(fd);
		} else {
			Logger::log(LOG_ERR, "Failed to acquire lock %s after retries", filePath.c_str());
			throw std::runtime_error("Failed to acquire lock: " + filePath);
		}

	}
	~Lock(){
		unlink(filePath.c_str());
	}
};

#endif /* LOCK_H_ */
