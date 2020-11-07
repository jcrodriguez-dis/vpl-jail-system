/**
 * @package:   Part of vpl-jail-system
 * @copyright: Copyright (C) 2014 Juan Carlos Rodríguez-del-Pino
 * @license:   GNU/GPL, see LICENSE.txt or http://www.gnu.org/licenses/gpl-3.0.html
 **/

#ifndef LOCK_H_
#define LOCK_H_
#include <string>
#include <fcntl.h>
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
		int ntry=0;
		int fd;
		while ( (fd=open(filePath.c_str(),O_CREAT|O_EXCL|O_WRONLY, 0x600)) == -1
				&& ntry <1000){
			time_t lastModification = Util::timeOfFileModification(filePath);
			if (lastModification &&  (lastModification + 2 < time(NULL))) {
				unlink(filePath.c_str());
				syslog(LOG_DEBUG,"locking timeout %s",filePath.c_str());
			}
			usleep(5000);
			ntry++;
		}
		if (fd > 0) {
			close(fd);
		}

	}
	~Lock(){
		unlink(filePath.c_str());
	}
};

#endif /* LOCK_H_ */
