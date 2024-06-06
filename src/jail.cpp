/**
 * @package:   Part of vpl-jail-system
 * @copyright: Copyright (C) 2021 Juan Carlos Rodríguez-del-Pino
 * @license:   GNU/GPL, see LICENSE.txt or http://www.gnu.org/licenses/gpl-3.0.html
 **/

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <climits>
#include <limits>
#include <cstring>
#include <algorithm>
#include <exception>
#include <sys/types.h>
#include <dirent.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pty.h>
#include "jail.h"
#include "redirector.h"
#include "httpServer.h"
#include "util.h"
#include "processMonitor.h"
#include "websocket.h"
/*
 * @return string "ready", "busy", "offline"
 */
string Jail::commandAvailable(long long memRequested){
	Logger::log(LOG_INFO,"Memory requested %lld", memRequested);
	if (memRequested <= 0) {
		return "ready";
	}
	if(Util::fileExists("/etc/nologin")){ //System going shutdown
		return "offline";
	}else {
		char *mem = (char*)malloc(memRequested);
		if(mem == NULL) return "busy";
		else free(mem);
	}
	return "ready";
}

void Jail::checkFilesNameCorrectness(mapstruct files) {
	for(mapstruct::iterator i = files.begin(); i != files.end(); i++) {
		if (! Util::correctPath(i->first)) {
			Logger::log(LOG_ERR,"Incorrect filename '%s'", i->first.c_str());
			throw HttpException(badRequestCode, "Incorrect file name");
		}
	}
}

void Jail::saveParseFiles(processMonitor &pm, RPC &rpc) {
	mapstruct files = rpc.getFiles();
	mapstruct fileencoding = rpc.getFileEncoding();
	//Save files to execution dir, decode data if needed
	for(mapstruct::iterator i = files.begin(); i != files.end(); i++){
		string name = i->first;
		string data = i->second->getString();
		if ( fileencoding.find(name) != fileencoding.end()
				&& fileencoding[name]->getInt() == 1 ) {
			Logger::log(LOG_INFO, "Decoding file %s from b64", name.c_str());
			data = Base64::decode(data);
			if ( name.length() > 4 && name.substr(name.length() - 4, 4) == ".b64") {
				name = name.substr(0, name.length() - 4);
			}
		}
		Logger::log(LOG_INFO, "Write file %s data size %lu", name.c_str(), (long unsigned int)data.size());
		pm.writeFile(name, data);
	}
}

ExecutionLimits Jail::getParseExecutionLimits(RPC &rpc) {
	mapstruct parsedata = rpc.getData();
	ExecutionLimits executionLimits = Configuration::getConfiguration()->getLimits();
	Logger::log(LOG_INFO,"Reading parms");
	executionLimits.log("Config");
	const TreeNode* maxtime = parsedata["maxtime"];
	const TreeNode* maxfilesize = parsedata["maxfilesize"];
	const TreeNode* maxmemory = parsedata["maxmemory"];
	const TreeNode* maxprocesses = parsedata["maxprocesses"];
	if (maxtime != NULL) {
		executionLimits.maxtime = min(maxtime->getInt(), executionLimits.maxtime);
	}
	if (maxfilesize != NULL) {
		executionLimits.maxfilesize = min(Util::fixMemSize(maxfilesize->getLong()),
                                      	executionLimits.maxfilesize);
	}
	if (maxmemory != NULL) {
		executionLimits.maxmemory = min(Util::fixMemSize(maxmemory->getLong()),
								    	executionLimits.maxmemory);
	}
	if (maxprocesses != NULL) {
		executionLimits.maxprocesses = min(maxprocesses->getInt(), executionLimits.maxprocesses);
	}
	executionLimits.log("Request");
	return executionLimits;
}

void Jail::commandRequest(RPC &rpc, string &adminticket,string &monitorticket,string &executionticket){
	mapstruct parsedata = rpc.getData();
	Logger::log(LOG_INFO,"Request for process");
	checkFilesNameCorrectness(rpc.getFiles());
	checkFilesNameCorrectness(rpc.getFileToDelete());
	processMonitor pm(adminticket, monitorticket, executionticket);
	// TODO: Cheking file names before the fork will give a proper response to the caller or not?
	pid_t pid=fork();
	if(pid==0){ //new process
		try {
			Logger::log(LOG_INFO,"Parse data %lu", (long unsigned int)parsedata.size());
			saveParseFiles(pm, rpc);
			Logger::log(LOG_INFO,"Reading parms");
			ExecutionLimits executionLimits = getParseExecutionLimits(rpc);
			string vpl_lang = parsedata["lang"]->getString();
			Logger::log(LOG_DEBUG, "VPL_LANG %s", vpl_lang.c_str());
			bool interactive = parsedata["interactive"]->getInt() > 0;
			Logger::log(LOG_DEBUG, "interactive %d", parsedata["interactive"]->getInt());
			pm.setExtraInfo(executionLimits, interactive, vpl_lang);
			pm.setCompiler();
			Logger::log(LOG_INFO, "Compilation");
			string script = parsedata["execute"]->getString();
			string compilationOutput = run(pm, script);
			pm.setCompilationOutput(compilationOutput);
			bool compiled = pm.FileExists(VPL_EXECUTION) || pm.FileExists(VPL_WEXECUTION) || pm.FileExists(VPL_WEBEXECUTION);
			if (compiled) {
				//Delete files
				mapstruct filestodelete = rpc.getFileToDelete();
				for (mapstruct::iterator i = filestodelete.begin(); i != filestodelete.end(); i++) {
					string name = i->first;
					Logger::log(LOG_INFO, "Delete file %s", name.c_str());
					pm.deleteFile(name);
				}
				if (!interactive && pm.FileExists(VPL_EXECUTION)) {
					pm.setRunner();
					Logger::log(LOG_INFO, "Non interactive execution");
					string program;
					if (pm.installScript(".vpl_launcher.sh", "vpl_batch_launcher.sh")) {
						program = ".vpl_launcher.sh";
					} else {
						program = VPL_EXECUTION;
					}
					string executionOutput = run(pm, program);
					Logger::log(LOG_INFO, "Write execution result");
					pm.setExecutionOutput(executionOutput, true);
				}
			}else{
				Logger::log(LOG_INFO, "Compilation fail");
			}
		}
		catch(std::exception &e){
			Logger::log(LOG_ERR, "unexpected exception: %s %s:%d", e.what(), __FILE__, __LINE__);
			_exit(EXIT_FAILURE);
		}
		catch(string &e){
			Logger::log(LOG_ERR, "unexpected exception: %s %s:%d", e.c_str(), __FILE__, __LINE__);
			_exit(EXIT_FAILURE);
		}
		catch(HttpException &e){
			Logger::log(LOG_ERR, "unexpected exception: %s %s:%d", e.getLog().c_str(), __FILE__, __LINE__);
			_exit(EXIT_FAILURE);
		}
		catch(const char *e){
			Logger::log(LOG_ERR, "unexpected exception: %s %s:%d", e,__FILE__, __LINE__);
			_exit(EXIT_FAILURE);
		}
		catch(...){
			Logger::log(LOG_ERR, "unexpected exception %s:%d", __FILE__, __LINE__);
			_exit(EXIT_FAILURE);
		}
		_exit(EXIT_SUCCESS);
	}
}

void Jail::commandDirectRun(RPC &rpc, string &homepath, string &adminticket, string &executionticket){
	mapstruct parsedata = rpc.getData();
	Logger::log(LOG_INFO,"Request for direct run");
	checkFilesNameCorrectness(rpc.getFiles());
	checkFilesNameCorrectness(rpc.getFileToDelete());
	processMonitor pm(adminticket, executionticket);
	homepath = pm.getRelativeHomePath();
	pid_t pid = fork();
	if (pid == 0) { //new process
		try {
			Logger::log(LOG_INFO,"Parse data %lu", (long unsigned int)parsedata.size());
			saveParseFiles(pm, rpc);
			Logger::log(LOG_INFO,"Reading parms");
			ExecutionLimits executionLimits = getParseExecutionLimits(rpc);
			string vpl_lang = parsedata["lang"]->getString();
			Logger::log(LOG_DEBUG, "VPL_LANG %s", vpl_lang.c_str());
			bool interactive = true;
			pm.setExtraInfo(executionLimits, interactive, vpl_lang);
			pm.setCompiler();
			Logger::log(LOG_INFO, "Compilation");
			string script = parsedata["execute"]->getString();
			string compilationOutput = run(pm, script);
			pm.setCompilationOutput(compilationOutput);
			bool compiled = pm.FileExists(VPL_EXECUTION);
			if (compiled) {
				//Delete files
				mapstruct filestodelete = rpc.getFileToDelete();
				for (mapstruct::iterator i = filestodelete.begin(); i != filestodelete.end(); i++) {
					string name = i->first;
					Logger::log(LOG_INFO, "Delete file %s", name.c_str());
					pm.deleteFile(name);
				}
			}else{
				Logger::log(LOG_INFO, "Compilation fail");
			}
		}
		catch(std::exception &e){
			Logger::log(LOG_ERR, "unexpected exception: %s %s:%d", e.what(), __FILE__, __LINE__);
			_exit(EXIT_FAILURE);
		}
		catch(string &e){
			Logger::log(LOG_ERR, "unexpected exception: %s %s:%d", e.c_str(), __FILE__, __LINE__);
			_exit(EXIT_FAILURE);
		}
		catch(HttpException &e){
			Logger::log(LOG_ERR, "unexpected exception: %s %s:%d", e.getLog().c_str(), __FILE__, __LINE__);
			_exit(EXIT_FAILURE);
		}
		catch(const char *e){
			Logger::log(LOG_ERR, "unexpected exception: %s %s:%d", e,__FILE__, __LINE__);
			_exit(EXIT_FAILURE);
		}
		catch(...){
			Logger::log(LOG_ERR, "unexpected exception %s:%d", __FILE__, __LINE__);
			_exit(EXIT_FAILURE);
		}
		_exit(EXIT_SUCCESS);
	}
}


void Jail::commandGetResult(string adminticket, string &compilation,
                            string &execution, bool &executed,
							bool &interactive){
	processMonitor pm(adminticket);
	pm.getResult(compilation, execution, executed);
	interactive = pm.isInteractive();
}

bool Jail::commandUpdate(string adminticket, RPC &rpc){
	checkFilesNameCorrectness(rpc.getFiles());
	processMonitor pm(adminticket);
	try {
		mapstruct files = rpc.getFiles();
		Logger::log(LOG_INFO,"parse files %lu", (long unsigned int)files.size());
		mapstruct fileencoding = rpc.getFileEncoding();
		//Save files to execution dir and options, decode data if needed
		for(mapstruct::iterator i = files.begin(); i != files.end(); i++){
			string name = i->first;
			string data = i->second->getString();
			if ( fileencoding.find(name) != fileencoding.end()
					&& fileencoding[name]->getInt() == 1 ) {
				Logger::log(LOG_INFO, "Decoding file %s from b64", name.c_str());
				data = Base64::decode(data);
				if ( name.length() > 4 && name.substr(name.length() - 4, 4) == ".b64") {
					name = name.substr(0, name.length() - 4);
				}
			}
			Logger::log(LOG_INFO, "Write file %s data size %lu", name.c_str(), (long unsigned int)data.size());
			pm.writeFile(name, data);
		}
		return true;
	}
	catch(std::exception &e){
		Logger::log(LOG_ERR, "unexpected exception: %s %s:%d", e.what(), __FILE__, __LINE__);
	}
	catch(string &e){
		Logger::log(LOG_ERR, "unexpected exception: %s %s:%d", e.c_str(), __FILE__, __LINE__);
	}
	catch(HttpException &e){
		Logger::log(LOG_ERR, "unexpected exception: %s %s:%d", e.getLog().c_str(), __FILE__, __LINE__);
	}
	catch(const char *e){
		Logger::log(LOG_ERR, "unexpected exception: %s %s:%d", e,__FILE__, __LINE__);
	}
	catch(...){
		Logger::log(LOG_ERR, "unexpected exception %s:%d", __FILE__, __LINE__);
	}
	return false;
}

bool Jail::commandRunning(string adminticket){
	try{
		processMonitor pm(adminticket);
		bool running = pm.isRunnig();
		if ( ! running ) {
			pm.cleanTask();
		}
		return running;
	}catch(...){
		return false;
	}
}
void Jail::commandStop(string adminticket){
	pid_t pid=fork();
	if(pid==0){ //new process
		try{
			processMonitor pm(adminticket);
			pm.cleanTask();
		}catch(...){

		}
		_exit(EXIT_SUCCESS);
	}
}
void Jail::commandMonitor(string monitorticket, Socket *s){
	processMonitor pm(monitorticket);
	webSocket ws(s);
	processState state = prestarting;
	bool webserver = false;
	bool runBrowser = false;
	time_t startTime = 0;
	time_t lastMessageTime = 0;
	time_t lastTime = pm.getStartTime();
	string lastMessage;
	while (state != stopped) {
		processState newstate = pm.getState();
		time_t now = time(NULL);
		time_t timeout = pm.getStartTime() + pm.getMaxTime();
		if( ! lastMessage.empty() && now != lastMessageTime){
			ws.send(lastMessage + ": "+Util::itos(now-startTime)+" seg");
			lastMessageTime = now;
		}
		if (newstate != state) {
			state = newstate;
			switch(state) {
			case prestarting:
				break;
			case starting:
				Logger::log(LOG_DEBUG,"Monitor starting");
				startTime = now;
				lastMessageTime = now;
				lastMessage = "message:starting";
				ws.send(lastMessage);
				break;
			case compiling:
				Logger::log(LOG_DEBUG,"Monitor compiling");
				timeout = now + pm.getMaxTime();
				startTime = now;
				lastMessageTime = now;
				lastMessage = "message:compilation";
				ws.send(lastMessage);
				break;
			case beforeRunning:
				Logger::log(LOG_DEBUG,"Monitor beforeRunning");
				ws.send("compilation:" + pm.getCompilation());
				timeout = now + JAIL_SOCKET_TIMEOUT;
				if (pm.FileExists(VPL_EXECUTION)) {
					Logger::log(LOG_DEBUG, "run:terminal");
					ws.send("run:terminal");
				} else if (pm.FileExists(VPL_WEBEXECUTION)) {
					Logger::log(LOG_DEBUG, "run:webterminal");
					ws.send("run:webterminal");
					webserver = true;
				} else if (pm.FileExists(VPL_WEXECUTION)) {
					ws.send("run:vnc:" + pm.getVNCPassword());
				} else {
					if (pm.getCompilation().empty()) {
						ws.send("compilation:The compilation process did not generate an executable nor error message.");
					}
					ws.send("close:");
					ws.close();
					ws.wait(500); //wait client response
					pm.cleanTask();
					ws.receive();
					return;
				}
				break;
			case running:
				Logger::log(LOG_DEBUG,"Monitor running");
				startTime = now;
				timeout = now + pm.getMaxTime() + 6 /* execution cleanup */;
				lastMessageTime = now;
				lastMessage = "message:running";
				ws.send(lastMessage);
				// TODO Checks if browser with cookie running to remove URL message
				if ( webserver && !runBrowser && pm.FileExists(VPL_LOCALSERVERADDRESSFILE) ) {
					runBrowser = true;
					ws.send("run:browser:" + pm.getHttpPassthroughTicket());
				}
				break;
			case retrieve:
				Logger::log(LOG_DEBUG, "Monitor retrieve");
				startTime = now;
				timeout = now + JAIL_HARVEST_TIMEOUT;
				ws.send("retrieve:");
				break;
			case stopped:
				Logger::log(LOG_DEBUG, "Monitor stopped");
				ws.send("close:");
				ws.close();
				ws.wait(500); //wait client response
				break;
			}
		}
		ws.wait(100); // 10 times a second
		
		string rec = ws.receive();
		if (ws.isClosed()) {
			if (state == retrieve && timeout >= time(NULL)) {
				continue;
			}
			break;
		}

		if (rec.size() > 0) { //Receive client close ws
			ws.close();
			break;
		}

		//Check running timeout
		if (state != starting && timeout < time(NULL)) {
			ws.send("message:timeout");
			usleep(3000000);
			ws.send("close:");
			ws.close();
			ws.wait(500); //wait client response
			ws.receive();
			break;
		}

		if (lastTime != now && pm.isOutOfMemory()) { //Every second check memory usage
			string ml= pm.getMemoryLimit();
			Logger::log(LOG_DEBUG,"Out of memory (%s)",ml.c_str());
			ws.send("message:outofmemory:"+ml);
			usleep(1500000);
			usleep(1500000);
			ws.send("close:");
			ws.close();
			ws.wait(500); //wait client response
			ws.receive();
			break;
		}
		lastTime = now;
	}
	pm.cleanTask();
}

void Jail::commandExecute(string executeticket, Socket *s){
	processMonitor pm(executeticket);
	webSocket ws(s);
	if (pm.getSecurityLevel() != execute){ 
		Logger::log(LOG_ERR,"%s: Security. Try to execute request with no monitor ticket",IP.c_str());
		throw "Internal server error";
	}
	Logger::log(LOG_INFO,"Start executing");
	if (pm.getState() == beforeRunning) { 
		pm.setRunner();
		if (pm.FileExists(VPL_EXECUTION)) {
			string program;
			if (pm.installScript(".vpl_launcher.sh", "vpl_terminal_launcher.sh")) {
				program = ".vpl_launcher.sh";
			} else {
				program = VPL_EXECUTION;
			}
			runTerminal(pm, ws, program);
		} else if (pm.FileExists(VPL_WEBEXECUTION)) {
			string program;
			if (pm.installScript(".vpl_launcher.sh", "vpl_web_launcher.sh")) {
				program = ".vpl_launcher.sh";
			} else {
				program = VPL_WEBEXECUTION;
			}
			runTerminal(pm, ws, program);
		} else if (pm.FileExists(VPL_WEXECUTION)) {
			if (pm.installScript(".vpl_launcher.sh", "vpl_vnc_launcher.sh"))
				runVNC(pm, ws, ".vpl_launcher.sh");
			else
				Logger::log(LOG_ERR, "%s:Error: vpl_vnc_launcher.sh not installed", IP.c_str());
		} else {
			Logger::log(LOG_ERR, "%s:Error: nothing to run", IP.c_str());
		}
	}
}

bool Jail::commandSetPassthroughCookie(string passthroughticket, HttpJailServer &server){
	try {
		processMonitor pm(passthroughticket);
		if ( pm.getState() != processState::running ||
		     pm.getSecurityLevel() != httppassthrough) {
			 return false;
		}
	} catch(...) {
		return false;
	}

	string extra = VPL_SETWEBCOOKIE + passthroughticket + "; Path=/\r\n";
	extra += VPL_LOCALREDIRECT;
	server.send(302, "REDIRECT", "", false, extra);
	return true;
}

/**
 * Passthrough the request to the localwebserver
 * 
 * @param socket The open socket that manage the connection
 * 
 * @return if the request finnaly has rigth to access
 */
bool Jail::httpPassthrough(string passthroughticket, Socket *socket){
	try {
		processMonitor pm(passthroughticket);
		if ( pm.getState() != processState::running ) {
			Logger::log(LOG_INFO,"httpPassthrough fail: ! processState::running");
			return false;
		}
		if ( pm.getSecurityLevel() != httppassthrough ) {
			Logger::log(LOG_INFO,"httpPassthrough fail: pm.getSecurityLevel() != httppassthrough");
			return false;
		}
		runPassthrough(pm, socket);
		return true;
	} catch(HttpException &exception){
		Logger::log(LOG_ERR,"%s:%s",IP.c_str(),exception.getLog().c_str());
		return false;
	}
	catch(std::exception &e){
		Logger::log(LOG_ERR,"%s:Unexpected exception %s on %s:%d",IP.c_str(), e.what(),__FILE__,__LINE__);
	}
	catch(string &exception){
		Logger::log(LOG_ERR,"%s:%s",IP.c_str(),exception.c_str());
	}
	catch(const char *s){
		Logger::log(LOG_ERR,"%s:%s",IP.c_str(),s);
	}catch(...) {
		Logger::log(LOG_INFO,"httpPassthrough fail: unexpected exception");
	}
	return false;
}

/**
 * Checks for http Passthrough setting request
 * 
 * @param URLPath URL path to check
 * @param ticket possible ticket found
 * 
 * @return true if valid request structure found
 */
bool Jail::isRequestingCookie(string URLPath, string &ticket) {
	static vplregex reg("^\\/([^\\/]+)\\/httpPassthrough$");
	vplregmatch match(3);
	if (! reg.search(URLPath, match)) {
		return false;
	}
	ticket = match[1];
	return true;
}

/**
 * Returns if IP can request a task
 * @return true if IP is allow to request task
 */
bool Jail::isValidIPforRequest() {
	const vector<string> &dirs = configuration->getTaskOnlyFrom();
	if (dirs.size() == 0) return true;
	int l = (int) dirs.size();
	for(int i = 0; i< l; i++){
		if(IP.find(dirs[i]) == 0)
			return true;
	}
	return false;
}


/**
 * Constructor, parameters => main parameters
 */
Jail::Jail(string IP){
	this->IP = IP;
	this->newpid = -1;
	this->redirectorpid = -1;
	configuration = Configuration::getConfiguration();
}

string Jail::predefinedURLResponse(string URLPath) {
	string page;
	if( Util::toUppercase(URLPath) == "/OK"){
		page = "<!DOCTYPE html><html><body><h1>OK</h1></body></html>";
	} else if( URLPath == "/robots.txt"){
		page = "User-agent: *\nDisallow: /\n";
	} else if( URLPath == "/favicon.ico"){
		page = "<svg viewBox=\"0 0 25 14\" xmlns=\"http://www.w3.org/2000/svg\">"
				"<style> .logo { font: bold 12px Arial Rounded MT,Arial,Helvetica;"
				"fill: #277ab0; stroke: black; stroke-width: 0.7px;} </style>"
				"<text x=\"0\" y=\"12\" class=\"logo\">VPL</text></svg>";
	} else if(configuration->getCerbotWebrootPath() != "") {
		static vplregex challenge("^\\/\\.well-known\\/acme-challenge\\/([^\\/]*)$");
		static vplregmatch path(2);
		if (challenge.search(URLPath, path)) {
			string filePath = configuration->getCerbotWebrootPath() + URLPath;
			if (Util::fileExists(filePath)) {
				page = Util::readFile(filePath, false);
			}
		}
	}
	return page;
}
/**
 * Process request
 *  3) read http request and header
 *  4) select http xmlrpc or websocket
 *  5) http: available, request, getresult, running, stop
 *  6) websocket: monitor, execute
 */
void Jail::process(Socket *socket){
	Logger::log(LOG_INFO, "Start server version %s", Util::version());
	string httpURLPath = configuration->getURLPath();
	HttpJailServer server(socket);
	try {
		socket->readHeaders();
		if(socket->headerSize() == 0){
			if(socket->isSecure()){ // Don't count SSL errors.
				_exit(static_cast<int>(neutral));
			}else{
				_exit(EXIT_FAILURE);
			}
		}
		if (socket->getCookie(VPL_WEBCOOKIE).length()) { // If found cookie passing to local application and stop processing
			if (httpPassthrough(socket->getCookie(VPL_WEBCOOKIE), socket)) {
				_exit(EXIT_SUCCESS);
			}
		}
		string httpMethod = socket->getMethod();
		if (Util::toUppercase(socket->getHeader("Upgrade")) != "WEBSOCKET") {
			Logger::log(LOG_INFO, "http(s) request");
			if (httpMethod == "GET") {
				ExitStatus securityStatus = ExitStatus::neutral;
				string response;
				string cookieTicket;
				if (isRequestingCookie(socket->getURLPath(), cookieTicket)) { // Getting cookie to access web app
					string iWasHere = socket->getCookie(VPL_IWASHERECOOKIE);
					if ( (!iWasHere.empty()) &&
					     (socket->getQueryString() == "private")) { // Requires a private browser
						response = "<html><body><h1>Please, to access web applications use a new Private or Incognito window.</h1></body></html>";
					} else {
						if (! commandSetPassthroughCookie(cookieTicket, server)) {
							securityStatus = ExitStatus::httpError;
						}
					}
				}
				if ( response.empty() ) {
					response = predefinedURLResponse(socket->getURLPath());
				}
				if ( response.empty() ) {
					throw HttpException(notFoundCode, "HTTP GET: URL path not found");
				}
				server.send200(response, false, VPL_SETIWASHERECOOKIE);
				_exit(static_cast<int>(securityStatus));
			}
			if (httpMethod == "HEAD") {
				string response = predefinedURLResponse(socket->getURLPath());
				if ( response.size() == 0 ) {
					throw HttpException(notFoundCode, "HTTP HEAD: URL path not found '" + socket->getURLPath() + "'");
				}
				server.send200(response, true);
				_exit(static_cast<int>(neutral));
			}
			if (httpMethod != "POST") {
				throw HttpException(badRequestCode, "Http(s) Unsupported METHOD " + httpMethod);
			}
			if (!isValidIPforRequest()) {
				throw HttpException(badRequestCode, "Client not allowed");
			}
			server.validateRequest(httpURLPath);
			string data = server.receive();
			RPC *prpc;
			if (data[0] == '<') {
				prpc = new XMLRPC(data);
			} else {
				prpc = new JSONRPC(data);
			}
			RPC& rpc = *prpc;
			string request = rpc.getMethodName();
			mapstruct parsedata = rpc.getData();
			Logger::log(LOG_INFO, "Execute request '%s'", request.c_str());
			if (request == "available") {
				ExecutionLimits jailLimits = configuration->getLimits();
				long long memRequested = parsedata["maxmemory"]->getLong();
				// Next line fixes XML-RPC int limits (-1).
				memRequested = memRequested > 0 ? memRequested : configuration->getLimits().maxmemory;
				string status = commandAvailable(memRequested);
				Logger::log(LOG_INFO, "Status: '%s'", status.c_str());
				server.send200(rpc.availableResponse(status,
				        processMonitor::requestsInProgress(),
						jailLimits.maxtime,
						jailLimits.maxfilesize,
						jailLimits.maxmemory,
						jailLimits.maxprocesses,
						configuration->getSecurePort()));
			} else if(request == "request") {
				string adminticket, monitorticket, executionticket;
				commandRequest(rpc, adminticket, monitorticket, executionticket);
				server.send200(rpc.requestResponse(adminticket,
								monitorticket,executionticket,
								configuration->getPort(),
								configuration->getSecurePort()));
			} else if(request == "directrun") {
				string homepath, adminticket, executionticket;
				commandDirectRun(rpc, homepath, adminticket, executionticket);
				server.send200(rpc.directRunResponse(homepath,
								adminticket,
								executionticket,
								configuration->getPort(),
								configuration->getSecurePort()));
			}else if(request == "getresult") {
				string adminticket, compilation, execution;
				bool executed,interactive;
				adminticket=parsedata["adminticket"]->getString();
				commandGetResult(adminticket, compilation, execution, executed, interactive);
				server.send200(rpc.getResultResponse(compilation, execution, executed, interactive));
			} else if(request == "running") {
				string adminticket;
				adminticket=parsedata["adminticket"]->getString();
				bool running = commandRunning(adminticket);
				if (! running && parsedata.count("pluginversion") == 0) {
					// Restores behaviour of < 2.3 version
					throw "Ticket invalid";
				}
				server.send200(rpc.runningResponse(running));
			} else if(request == "stop") {
				string adminticket;
				adminticket=parsedata["adminticket"]->getString();
				commandStop(adminticket);
				server.send200(rpc.stopResponse());
			} else if(request == "update") {
				string adminticket=parsedata["adminticket"]->getString();
				bool ok = commandUpdate(adminticket, rpc);
				server.send200(rpc.updateResponse(ok));
			} else { //Error
				throw HttpException(badRequestCode, "Unknown request:" + request);
			}
			delete prpc;
		} else { //Websocket
			if(socket->getMethod() != "GET"){
				throw "ws(s) Unsupported METHOD " + httpMethod;
			}
			string URLPath = socket->getURLPath();
			vplregex webSocketPath("^\\/([^\\/]+)\\/(.+)$");
			vplregmatch found(3);
			bool isfound = webSocketPath.search(URLPath, found);
			if (! isfound) {
				throw string("Bad URL");
			}
			string ticket = found[1];
			string command = found[2];
			if (command == "monitor") {
				commandMonitor(ticket, socket);
			} else if (command == "execute") {
				commandExecute(ticket, socket);
			} else {
				throw string("Bad command");
			}
		}
		_exit(EXIT_SUCCESS);
	}
	catch(HttpException &exception){
		Logger::log(LOG_INFO, "%s:%s", IP.c_str(), exception.getLog().c_str());
		server.sendCode(exception.getCode(), exception.getMessage());
		_exit(EXIT_SUCCESS);
	}
	catch(std::exception &e){
		Logger::log(LOG_WARNING, "%s:Unexpected exception %s on %s:%d", IP.c_str(), e.what(), __FILE__, __LINE__);
		server.sendCode(internalServerErrorCode, "Unknown error");
	}
	catch(string &exception){
		Logger::log(LOG_WARNING, "%s:%s",IP.c_str(), exception.c_str());
		server.sendCode(internalServerErrorCode, exception);
	}
	catch(const char *s){
		Logger::log(LOG_WARNING, "%s:%s",IP.c_str(), s);
		server.sendCode(internalServerErrorCode, s);
	}
	catch(...){
		Logger::log(LOG_WARNING, "%s:Unexpected exception %s:%d",IP.c_str(),__FILE__,__LINE__);
		server.sendCode(internalServerErrorCode,"Unknown error");
	}
	_exit(EXIT_FAILURE);
}

/**
 * chdir and chroot to jail
 */
void Jail::goJail(){
	string jailPath=configuration->getJailPath();
	if (jailPath == "") {
		Logger::log(LOG_INFO,"No chrooted, running in container");
		return;
	}
	if(chdir(jailPath.c_str()) != 0)
		throw HttpException(internalServerErrorCode, "I can't chdir to jail", jailPath);
	if(chroot(jailPath.c_str()) != 0)
		throw HttpException(internalServerErrorCode, "I can't chroot to jail", jailPath);
	Logger::log(LOG_INFO,"chrooted \"%s\"",jailPath.c_str());
}

/**
 * Execute program at prisoner home directory
 */
void Jail::transferExecution(processMonitor &pm, string fileName){
	string dir=pm.getRelativeHomePath();
	Logger::log(LOG_DEBUG, "Jail::transferExecution to %s+%s", dir.c_str(), fileName.c_str());
	string fullname = dir + "/" + fileName;
	if(chdir(dir.c_str())){
		throw "I can't chdir to exec dir :" + dir;
	}
	if(!Util::fileExists(fullname)){
		throw string("transferExecution: execution file not found: ") + fullname;
	}
	char *arg[6];
	char *command= new char[fullname.size() + 1];
	strcpy(command, fullname.c_str());
	setpgrp();
	int narg=0;
	arg[narg++] = command;
	arg[narg++] = NULL;
	int nenv = 0;
	char *env[10];
	string uid = Util::itos(pm.getPrisonerID());
	string HOME = "HOME=/home/p" + uid;
	env[nenv++] = (char *)HOME.c_str();
	string PATH = "PATH=" + configuration->getCleanPATH();
	env[nenv++] = (char *)PATH.c_str();
	env[nenv++] = (char *)"TERM=dumb";
	string UID = "UID=" + uid;
	env[nenv++]=(char *)UID.c_str();
	string USER = "USER=p" + uid;
	env[nenv++] = (char *)USER.c_str();
	string USERNAME = "USERNAME=Prisoner " + uid;
	env[nenv++] = (char *)USERNAME.c_str();
	string VPL_LANG = "VPL_LANG=" + pm.getLang();
	env[nenv++] = (char *)VPL_LANG.c_str();
	string VPL_VNCPASSWD = "VPL_VNCPASSWD=" + pm.getVNCPassword();
	if (pm.isInteractive()) {
		env[nenv++] = (char *)VPL_VNCPASSWD.c_str();
	}
	env[nenv++] = NULL;
	Logger::log(LOG_DEBUG, "Running \"%s\"", command);
	execve(command, arg, env);
	throw string("I can't execve: ") + command + " (" + strerror(errno) + ")";
}

/**
 * set user limits
 */
void Jail::setLimits(processMonitor &pm){
	ExecutionLimits executionLimits = pm.getLimits();
	executionLimits.log("setLimits");
	struct rlimit limit;
	limit.rlim_cur=0;
	limit.rlim_max=0;
	setrlimit(RLIMIT_CORE,&limit);
	limit.rlim_cur=executionLimits.maxtime;
	limit.rlim_max=executionLimits.maxtime;
	setrlimit(RLIMIT_CPU,&limit);
	if(executionLimits.maxfilesize>0){ //0 equals no limit
		limit.rlim_cur=executionLimits.maxfilesize;
		limit.rlim_max=executionLimits.maxfilesize;
		setrlimit(RLIMIT_FSIZE,&limit);
	}
	if(executionLimits.maxprocesses>0){ //0 equals no change
		limit.rlim_cur=executionLimits.maxprocesses;
		limit.rlim_max=executionLimits.maxprocesses;
		setrlimit(RLIMIT_NPROC,&limit);
	}
	//RLIMIT_MEMLOCK
	//RLIMIT_MSGQUEUE
	//RLIMIT_NICERLIMIT_NOFILE
	//RLIMIT_RTPRIO
	//RLIMIT_SIGPENDING
}

/**
 * run program controlling timeout and redirection
 */
string Jail::run(processMonitor &pm, string name, int othermaxtime, bool VNCLaunch){
	int maxtime;
	pm.getLimits().log("run");
	if (othermaxtime) {
		maxtime = othermaxtime;
		Logger::log(LOG_INFO, "Other maxtime set: %d", othermaxtime);
	}
	else
		maxtime = pm.getMaxTime();
	int fdmaster = -1;
	signal(SIGTERM, SIG_IGN);
	signal(SIGKILL, SIG_IGN);
	newpid = forkpty(&fdmaster, NULL, NULL, NULL);
	if (newpid == -1) //fork error
		return "Jail: fork error";
	if (newpid == 0) { //new process
		Logger::setForeground(false);
		try {
			goJail();
			setLimits(pm);
			pm.becomePrisoner();
			setsid();
			transferExecution(pm, name);
		} catch(const char *s) {
			Logger::log(LOG_ERR, "Error running: %s",s);
			printf("\nJail error: %s\n",s);
		} catch(const string &s) {
			Logger::log(LOG_ERR,"Error running: %s",s.c_str());
			printf("\nJail error: %s\n",s.c_str());
		} catch(...) {
			Logger::log(LOG_ERR,"Error running");
			printf("\nJail error: at execution stage\n");
		}
		_exit(EXIT_SUCCESS);
	}
	Logger::log(LOG_INFO, "child pid %d",newpid);
	RedirectorTerminalBatch redirector(fdmaster);
	time_t startTime = time(NULL);
	time_t lastTime = startTime;
	int stopSignal = SIGTERM;
	bool noMonitor = false;
	int status;
	while(redirector.isActive()) {
		redirector.advance();
		pid_t wret = waitpid(newpid, &status, WNOHANG);
		if (wret == newpid) {
			if(WIFSIGNALED(status)){
				int signal = WTERMSIG(status);
				char buf[1000];
				sprintf(buf,"\r\nJail: program terminated due to \"%s\" (%d)",
						strsignal(signal),signal);
				redirector.addMessage(buf);
			}else if(WIFEXITED(status)){
				int exitcode = WEXITSTATUS(status);
				if(exitcode != EXIT_SUCCESS){
					char buf[100];
					sprintf(buf,"\r\nJail: program terminated normally with exit code %d.\n",exitcode);
					redirector.addMessage(buf);
				}
			}else{
				redirector.addMessage("\r\nJail: program terminated but unknown reason.");
			}
			newpid = -1;
			break;
		} else if(wret > 0){//waitpid error wret != newpid
			redirector.addMessage("\r\nJail waitpid error: ret>0.\n");
			break;
		} else if(wret == -1) { //waitpid error
			redirector.addMessage("\r\nJail waitpid error: ret==-1.\n");
			Logger::log(LOG_INFO,"Jail waitpid error: %m");
			break;
		}
		if (wret == 0){ //Process running
			time_t now=time(NULL);
			if(lastTime != now){
				int elapsedTime = now - startTime;
				lastTime = now;
				if(elapsedTime > JAIL_MONITORSTART_TIMEOUT && !pm.isMonitored()){
					if(stopSignal != SIGKILL)
						redirector.addMessage("\r\nJail: browser connection error.\n");
					Logger::log(LOG_INFO,"Not monitored");
					kill(newpid, stopSignal);
					stopSignal = SIGKILL; //Second try
					noMonitor = true;
				} else if(elapsedTime > maxtime){
					redirector.addMessage("\r\nJail: execution time limit reached.\n");
					Logger::log(LOG_INFO,"Execution time limit (%d) reached"
							,maxtime);
					kill(newpid, stopSignal);
					stopSignal = SIGKILL; //Second try
				} else if(VNCLaunch && redirector.getOutputSize() > 0){
					redirector.advance();
					break;
				}else if(pm.isOutOfMemory()){
					string ml = pm.getMemoryLimit();
					if(stopSignal != SIGKILL)
						redirector.addMessage("\r\nJail: out of memory (" + ml + ")\n");
					Logger::log(LOG_INFO,"Out of memory (%s)", ml.c_str());
					kill(newpid, stopSignal);
					stopSignal = SIGKILL; //Second try
				}
			}
		}
	}
	//wait until 5sg for redirector to read and send program output
	int max_iter = VNCLaunch ? 5 : 50;
	for(int i=0; redirector.isActive() && i < max_iter; i++){
		redirector.advance();
		usleep(100000); // 1/10 seg
	}
	string output = redirector.getOutput();
	Logger::log(LOG_DEBUG,"Complete program output: %s", output.c_str());
	if(noMonitor && !VNCLaunch){
		pm.cleanTask();
	}
	return output;
}

/**
 * run program in terminal controlling timeout and redirection
 */
void Jail::runTerminal(processMonitor &pm, webSocket &ws, string name){
	int fdmaster = -1;
	ExecutionLimits executionLimits = pm.getLimits();
	signal(SIGTERM, SIG_IGN);
	signal(SIGKILL, SIG_IGN);
	newpid = forkpty(&fdmaster, NULL, NULL, NULL);
	if (newpid == -1) { //fork error
		Logger::log(LOG_INFO, "Jail: fork error %m");
		return;
	}
	if (newpid == 0) { //new process
		Logger::setForeground(false);
		try{
			goJail();
			setLimits(pm);
			pm.becomePrisoner();
			setsid();
			transferExecution(pm,name);
		}catch(const char *s){
			Logger::log(LOG_ERR,"Error running terminal: %s",s);
		}
		catch(...){
			Logger::log(LOG_ERR,"Error running terminal");
		}
		_exit(EXIT_SUCCESS);
	}
	Logger::log(LOG_INFO, "child pid %d",newpid);
	RedirectorTerminal redirector(fdmaster,&ws);
	Logger::log(LOG_INFO, "Redirector start terminal control");
	time_t startTime = time(NULL);
	time_t lastTime = startTime;
	int stopSignal = SIGTERM;
	int status;
	Logger::log(LOG_INFO,"run: start redirector loop");
	while(redirector.isActive() && !ws.isClosed()){
		redirector.advance();
		pid_t wret = waitpid(newpid, &status, WNOHANG);
		if(wret == 0){
			time_t now = time(NULL);
			if(lastTime != now){
				int elapsedTime = now-startTime;
				lastTime = now;
				if (elapsedTime > JAIL_MONITORSTART_TIMEOUT && !pm.isMonitored()) {
					Logger::log(LOG_INFO, "Not monitored");
					if(stopSignal != SIGKILL)
						redirector.addMessage("\r\nJail: process stopped\n");
					redirector.stop();
					kill(newpid, stopSignal);
					stopSignal = SIGKILL;
				} else if(elapsedTime > executionLimits.maxtime) {
					if (stopSignal != SIGKILL)
						redirector.addMessage("\r\nJail: execution time limit reached.\n");
					redirector.stop();
					Logger::log(LOG_INFO, "Execution time limit (%d) reached",
							executionLimits.maxtime);
					kill(newpid, stopSignal);
					stopSignal = SIGKILL;
				} else if (pm.isOutOfMemory()) {
					string ml= pm.getMemoryLimit();
					if (stopSignal != SIGKILL)
						redirector.addMessage("\r\nJail: out of memory (" + ml + ")\n");
					Logger::log(LOG_INFO, "Out of memory (%s)",ml.c_str());
					kill(newpid, stopSignal);
					stopSignal = SIGKILL; //Second try
				}
			}
		} else { //Not running or error
			break;
		}
	}
	Logger::log(LOG_DEBUG, "End redirector loop");
	//wait until 5sg for redirector to read and send program output
	for (int i = 0; redirector.isActive() && i < 50; i++) {
		redirector.advance();
		usleep(100000); // 1/10 seg
	}
	pm.cleanTask();
}

/**
 * run program in VNCserver controlling timeout and redirection
 */
void Jail::runVNC(processMonitor &pm, webSocket &ws, string name){
	ExecutionLimits executionLimits = pm.getLimits();
	signal(SIGTERM, SIG_IGN);
	signal(SIGKILL, SIG_IGN);
	string output = run(pm, name, 10, true);
	Logger::log(LOG_DEBUG,"VNC launch %s", output.c_str());
	int VNCServerPort = Util::atoi(output);
	RedirectorVNC redirector(&ws, VNCServerPort);
	Logger::log(LOG_INFO, "Redirector start vncserver control");
	time_t startTime=time(NULL);
	time_t lastTime=startTime;
	bool noMonitor=false;
	Logger::log(LOG_INFO,"run: start redirector loop");
	while(redirector.isActive() && !ws.isClosed()){
		redirector.advance();
		time_t now=time(NULL);
		if(lastTime != now){
			int elapsedTime=now-startTime;
			lastTime = now;
			//TODO report to user the out of resources
			if(elapsedTime > executionLimits.maxtime){
				Logger::log(LOG_INFO,"Execution time limit (%d) reached"
						,executionLimits.maxtime);
				redirector.stop();
				break;
			}
			if(pm.isOutOfMemory()){
				string ml= pm.getMemoryLimit();
				Logger::log(LOG_INFO,"Out of memory (%s)",ml.c_str());
				redirector.stop();
				break;
			}
			if(elapsedTime>JAIL_MONITORSTART_TIMEOUT && !pm.isMonitored()){
				Logger::log(LOG_INFO,"Not monitored");
				redirector.stop();
				noMonitor=true;
				break;
			}
		}
	}
	Logger::log(LOG_DEBUG,"End redirector loop");
	//wait until 5sg for redirector to read and send program output
	for(int i=0;redirector.isActive() && i<50; i++){
		redirector.advance();
		usleep(100000); // 1/10 seg
	}
	if(pm.installScript(".vpl_vnc_stopper.sh","vpl_vnc_stopper.sh")){
		output=run(pm,".vpl_vnc_stopper.sh",5); //FIXME use constant
		Logger::log(LOG_DEBUG,"%s",output.c_str());
	}
	if(noMonitor){
		pm.cleanTask();
	}
}

/**
 * Data Passthrough from navigator to local webserver
 */
void Jail::runPassthrough(processMonitor &pm, Socket *s) {
	// TODO change to config data
	string localServerAdress = pm.getLocalWebServer();
	RedirectorWebServer redirector(s, localServerAdress);
	Logger::log(LOG_INFO, "Redirector web server request start");
	time_t startTime = time(NULL);
	time_t lastTime = startTime;
	while (redirector.isActive() && ! s->isClosed()) {
		redirector.advance();
		time_t now = time(NULL);
		if (lastTime != now) {
			int elapsedTime = now - startTime;
			lastTime = now;
			if (elapsedTime > JAIL_HARVEST_TIMEOUT) {
				Logger::log(LOG_INFO,"Execution time limit (%d) reached", JAIL_HARVEST_TIMEOUT);
				s->send("<b>Execution time limit reached</b>");
				redirector.stop();
				break;
			}
		}
	}
	Logger::log(LOG_DEBUG,"End web redirector loop");
}

