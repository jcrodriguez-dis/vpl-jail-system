/**
 * @package:   Part of vpl-jail-system
 * @copyright: Copyright (C) 2014 Juan Carlos Rodríguez-del-Pino
 * @license:   GNU/GPL3, see LICENSE.txt or http://www.gnu.org/licenses/gpl-3.0.html
 **/

#ifndef JAIL_INC_H
#define JAIL_INC_H
#include "xmlrpc.h"
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
#include "xmlrpc.h"
#include "jsonrpc.h"

/**
 * class Jail
 **/
class Jail{
protected:
	pid_t  newpid;            //pid of program executed
	pid_t  redirectorpid;    //pid of redirector process
	string IP; //Client IP
	Configuration *configuration;
	static void catchSIGTERM(int n);
	void goJail();
	void changeUser(processMonitor &pm);
	void setLimits(processMonitor &pm);
	void transferExecution(processMonitor &pm,string fileName);
	void saveParseFiles(processMonitor &pm, RPC &rpc);
	ExecutionLimits getParseExecutionLimits(RPC &rpc);
	//Action commands
	string commandAvailable(long long memRequested);
	void commandRequest(RPC &rpc, string &adminticket,string &monitorticket,string &executionticket);
	void commandDirectRun(RPC &rpc, string &homepath, string &adminticket, string &executionticket);
	void commandGetResult(string adminticket,string &compilation,string &execution,bool &executed,bool &interactive);
	bool commandUpdate(string adminticket, RPC &rpc);
	bool commandRunning(string adminticket);
	void commandStop(string adminticket);
	void commandMonitor(string userticket,Socket *s);
	void commandExecute(string userticket,Socket *s);
	bool commandSetPassthroughCookie(string passthroughticket, HttpJailServer & server);
public:
	Jail(string);
	bool isValidIPforRequest();
	bool httpPassthrough(string passthroughticket, Socket *socket);
	bool isRequestingCookie(string URLPath, string &ticket);
	string predefinedURLResponse(string URLPath);
	void process(Socket *);//process request and return answer
	void writeFile(processMonitor &pm,string name, const string &data);
	string readFile(processMonitor &pm,string name);
	void deleteFile(processMonitor &pm,string name);
	string run(processMonitor &pm,string name, int othermaxtime=0);
	void runTerminal(processMonitor &pm, webSocket &s, string name);
	void runVNC(processMonitor &pm, webSocket &s, string name);
	void runPassthrough(processMonitor &pm, Socket *s);
};
#endif
