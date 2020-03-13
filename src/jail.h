/**
 * @package:   Part of vpl-jail-system
 * @copyright: Copyright (C) 2014 Juan Carlos Rodríguez-del-Pino
 * @license:   GNU/GPL3, see LICENSE.txt or http://www.gnu.org/licenses/gpl-3.0.html
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

#include "processMonitor.h"
#include "websocket.h"

/**
 * class Jail
 **/
class Jail{
	pid_t  newpid;            //pid of program executed
	pid_t  redirectorpid;    //pid of redirector process
	string IP; //Client IP
	Configuration *configuration;
	static void catchSIGTERM(int n);
	void goJail();
	void changeUser(processMonitor &pm);
	void setLimits(processMonitor &pm);
	void transferExecution(processMonitor &pm,string fileName);
	bool isValidIPforRequest();
	//Action commands
	string commandAvailable(int memRequested);
	void commandRequest(mapstruct &parsedata, string &adminticket,string &monitorticket,string &executionticket);
	void commandGetResult(string adminticket,string &compilation,string &execution,bool &executed,bool &interactive);
	bool commandRunning(string adminticket);
	void commandStop(string adminticket);
	void commandMonitor(string userticket,Socket *s);
	void commandExecute(string userticket,Socket *s);
	string predefinedURLResponse(string URLPath);
public:
	Jail(string);
	void process(Socket *);//process request and return answer
	void writeFile(processMonitor &pm,string name, const string &data);
	string readFile(processMonitor &pm,string name);
	void deleteFile(processMonitor &pm,string name);
	string run(processMonitor &pm,string name, int othermaxtime=0);
	void runTerminal(processMonitor &pm, webSocket &s, string name);
	void runVNC(processMonitor &pm, webSocket &s, string name);
};
#endif
