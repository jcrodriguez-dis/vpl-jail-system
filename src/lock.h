/**
 * @version:   $Id: lock.h,v 1.7 2014-02-21 18:13:30 juanca Exp $
 * @package:   Part of vpl-jail-system
 * @copyright: Copyright (C) 2013 Juan Carlos Rodríguez-del-Pino
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

using namespace std;

class Lock{
	string filePath;
public:
	Lock(string DirPath){
		filePath=DirPath+"/lock";
		int ntry;
		int fd;
		while((fd=open(filePath.c_str(),O_CREAT|O_EXCL|O_WRONLY, 0x600))==-1 && ntry <1000){
			struct stat fileInfo;
			if(stat(filePath.c_str(), &fileInfo)>=0)
				if(fileInfo.st_mtime+2 < time(NULL)){
					unlink(filePath.c_str());
					syslog(LOG_DEBUG,"locking timeout %s",filePath.c_str());
				}
			usleep(5000);
			ntry++;
		}
		if(fd>0)
			close(fd);

	}
	~Lock(){
		unlink(filePath.c_str());
	}
};

#endif /* LOCK_H_ */
