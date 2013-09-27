/**
 * version:		$Id: jail.h,v 1.6 2011-04-04 14:04:25 juanca Exp $
 * package:		Part of vpl-xmlrpc-jail
 * copyright:	Copyright (C) 2009 Juan Carlos Rodríguez-del-Pino. All rights reserved.
 * license:		GNU/GPL, see LICENSE.txt or http://www.gnu.org/licenses/gpl-2.0.html
 **/

#ifndef JAIL_INC_H
#define JAIL_INC_H
#include "rpc.h"
#include <string>
#include <fstream>
using namespace std;
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>
#include <signal.h>
#include <errno.h>
#include <pty.h>
#include <sys/wait.h>
#include <sys/resource.h>

/**
 * class Jail
 **/
class Jail{
	string jailPath;   //Path to jail file system
	string configPath; //Path to configuration file
	uid_t  minPrisoner, maxPrisoner; //uid prisoner selection limits
	uid_t  prisoner;   //User prisoner selected
	bool   needClean;	//true if need to clean prisoner
	struct Interactive{
		bool   requested; 	//true if run interactive is requested
		string sname;		//program to run
		int    host;		//host to connect
		int    port;		//port number
		string  password;	//password for connection
		Interactive(){
			requested=false;
		}
	} interactive;
	struct {
		int maxtime;
		int maxfilesize;
		int maxmemory;
		int maxprocesses;
	} executionLimits, jailLimits;
	string softwareInstalled;
	const int argc;
	const char ** const argv;
	char * const * const environment;
	pid_t  newpid;			//pid of program executed
	pid_t  redirectorpid;	//pid of redirector process
	void checkConfig();
	void checkJail();
	void checkPath(const string c,const int minSize=2);
	string prisonerHomePath();
	bool parseConfigLine(const string &line, string &param, string &svalue, int &value);
	void readConfigFile();
	void selectPrisoner();
	int  load();
	void stopPrisonerProcess();
	void removePrisonerHome();
	void processRequest(); //process request and return answer
	int removeDir(string dir, bool force);
	void clean(); //Clean prisoner process and home dir
	static void catchSIGTERM(int n);
	void goJail();
	void changeUser();
	void transferExecution(string fileName);
	void setLimits();

public:
	Jail(int const argcp, const char ** const argvp,char * const * const envp);
	~Jail();
	void   process();
	void   writeFile(string name, string data);
	string readFile(string name);
	void deleteFile(string name);
	string run(string name, int host=0, int port=0, string password="");
	void runInteractive(string name, int hostip, int port, string password);
};
#endif
