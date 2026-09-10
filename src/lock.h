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
#include <sys/file.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <syslog.h>

#include "util.h"
#include "configuration.h"

using namespace std;

class Lock{
	int fd;
	string filePath;
	static string getBaseDir() {
		Configuration* configuration = Configuration::getConfiguration();
		return configuration->getControlPath() + "/locks";
	}
public:
	Lock(const string &name, int operation = LOCK_EX) : fd(-1) {
		if (name.empty() || name.size() > 32) {
			throw runtime_error("Invalid lock name");
		}
		for (size_t i = 0; i < name.size(); i++) {
			char c = name[i];
			if (!((c >= 'a' && c <= 'z') ||
				  (c >= 'A' && c <= 'Z') ||
				  (c >= '0' && c <= '9') ||
				  (c == '_'))) {
				throw runtime_error("Invalid lock name");
			}
		}
		string baseDir = getBaseDir();
		if (!Util::dirExists(baseDir) && !Util::createDir(baseDir, 0, baseDir.size())) {
			throw runtime_error("Failed to create lock directory: " + baseDir);
		}
		filePath = baseDir + "/" + name + ".lock";
		fd = open(filePath.c_str(), O_CREAT|O_RDWR|O_CLOEXEC|O_NOFOLLOW, 0600);
		if (fd < 0) {
			throw runtime_error("Failed to open lock: " + filePath);
		}
		int result;
		do {
			result = flock(fd, operation);
		} while (result < 0 && errno == EINTR);
		if (result < 0) {
			int savedErrno = errno;
			close(fd);
			fd = -1;
			throw runtime_error("Failed to acquire lock " + filePath + ": " + strerror(savedErrno));
		}
	}
	Lock(const Lock&) = delete;
	Lock& operator=(const Lock&) = delete;
	~Lock(){
		if (fd >= 0) {
			flock(fd, LOCK_UN);
			close(fd);
		}
	}
};

class GlobalLock : public Lock {
public:
	GlobalLock(int operation = LOCK_EX) : Lock("global", operation) {}
};

class TaskLock : public Lock {
public:
	TaskLock(int taskId, int operation = LOCK_EX) : Lock("task_" + to_string(taskId)	, operation) {}
};
#endif /* LOCK_H_ */
