/**
 * package:		Part of vpl-jail-system
 * copyright:	Copyright (C) 2009 Juan Carlos Rodríguez-del-Pino. All rights reserved.
 * license:		GNU/GPL, see LICENSE.txt or http://www.gnu.org/licenses/gpl-3.0.html
 **/

/**
 * @file vpl-jail-server.h
 * @brief Main server/daemon accept loop for vpl-jail-system.
 *
 * This header contains the `Daemon` class that:
 * - Opens one or two listening sockets (plain HTTP and/or HTTPS).
 * - Accepts incoming connections and `fork()`s a child process per request.
 * - Tracks active children and harvests exit status to maintain statistics.
 * - Enforces basic integrity checks on the jail filesystem and control directory.
 *
 * Request handling model
 * - The parent process remains privileged and only performs: accept/poll, book-keeping,
 *   and periodic maintenance.
 * - Each accepted connection is handled in a forked child which constructs a `Jail`
 *   instance and calls `Jail::process(Socket*)`. The child then `_exit()`s.
 * - Children communicate success/failure back to the parent exclusively via their
 *   process exit code (`ExitStatus`). This is consumed by `harvest()`.
 *
 * Security invariants checked at startup
 * - Jail `/etc` directory and critical account files must be owned by root and must not
 *   be writable by group/others.
 * - The control directory (from configuration) must be owned by root and not writable
 *   by group/others.
 *
 * Fail2ban-style throttling
 * - The server keeps an in-memory per-IP log of request/error counts.
 * - If enabled by configuration, an IP may be rejected when its error rate exceeds
 *   a threshold (see `isBanned`). Logs are periodically cleared to keep memory bounded.
 *
 * Notes
 * - Despite being a header, this file contains full implementation for historical reasons.
 * - No threading is used; concurrency is achieved via `fork()`.
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <memory>
#include <map>
#include <list>
#include <arpa/inet.h>
using namespace std;
#include "jail.h"
#include "util.h"

/**
 * @brief Singleton server acceptor and child-process supervisor.
 *
 * Typical lifecycle:
 * 1) `getRunner()` constructs the singleton and performs startup checks.
 * 2) `daemonize()` or `foreground()` optionally detaches and writes a PID file.
 * 3) `loop()` runs the accept/poll loop until SIGTERM requests a graceful shutdown.
 */
class Daemon {
	Configuration *configuration;
	static Daemon* singlenton;
	int port,sport;
	int listenSocket;
	int secureListenSocket;
	int actualSocket;
	int nSockets;
	struct pollfd sockets[2];
	bool isSecureSocket[2];
	string interface;
	//Access statistics
	struct {
		int requests;
		int errors;
		int rejected;
	} statistics;
	//Error log by IP
	struct Log{
		int requests;
		int errors;
		Log(){
			requests=0;
			errors=0;
		}
	};
	map<string,Log> logs;
	//Live tasks by pid
	struct Child{
		string IP;
		time_t start;
	};
	map<pid_t,Child> children;
	static bool finishRequested;
	/**
	 * SIGTERM handler: request termination of the main loop.
	 *
	 * The handler is intentionally minimal and async-signal-safe: it only sets a flag
	 * checked by `loop()`.
	 */
	static void SIGTERMHandler(int) {
		Daemon::finishRequested = true;
	}

	/**
	 * Validate jail filesystem integrity.
	 *
	 * When `JAILPATH` is configured (not running in a container), this checks that
	 * the jail's `/etc` and key account files are owned by root and are not writable
	 * by group/others.
	 *
	 * @throws const char* on failure (historical style used across the codebase).
	 */
	void checkJail(){
		string jailPath = configuration->getJailPath();
		if (jailPath == "") { // Running in container
			return;
		}
		struct stat info;

		string detc = jailPath + "/etc";
		if(lstat(detc.c_str(), &info))
			throw "jail /etc not checkable (Running in container no privileged with JAILPATH != /?)";
		if(info.st_uid !=0 || info.st_gid !=0)
			throw "jail /etc not owned by root";
		if(info.st_mode & 022)
			throw "Jail error: jail /etc with insecure permissions (group or others with write permission)";

		string fpasswd = jailPath + "/etc/passwd";
		if(lstat(fpasswd.c_str(),&info))
			throw "Jail error: jail /etc/passwd not checkable";
		if(info.st_uid !=0 || info.st_gid !=0)
			throw "Jail error: jail /etc/passwd not owned by root";
		if(info.st_mode & 033)
			throw "Jail error: jail /etc/passwd with insecure permissions (group or others with write permission)";

		string fgroup = jailPath + "/etc/group";
		if(lstat(fgroup.c_str(),&info))
			throw "Jail error: jail /etc/group not checkable";
		if(info.st_uid !=0 || info.st_gid !=0)
			throw "Jail error: jail /etc/group not owned by root";
		if(info.st_mode & 033)
			throw "Jail error: jail /etc/group with insecure permissions (group or others with write permission)";

		string fshadow = jailPath + "/etc/shadow";
		if(!lstat(fshadow.c_str(),&info)) {
			if(info.st_uid !=0 || info.st_gid !=0)
				throw "Jail error: jail /etc/shadow not owned by root";
			if(info.st_mode & 077)
				throw "Jail error: jail /etc/shadow with insecure permissions (group or others with some permission)";
		}
	}

	/**
	 * Validate the control directory integrity.
	 *
	 * The control directory stores per-task control/status data and must not be writable
	 * by group/others.
	 *
	 * @throws string on failure.
	 */
	void checkControlDir(){
		string cd=configuration->getControlPath();
		struct stat info;
		if(lstat(cd.c_str(),&info))
			throw "control path not checkable: "+cd;
		if(info.st_uid !=0 || info.st_gid !=0)
			throw "control path not owned by root: "+cd;
		if(info.st_mode & 022)
			throw "control path with insecure permissions (must be 0x44)";
	}

	/**
	 * @brief Return whether a client IP should be rejected.
	 *
	 * This is a lightweight, in-memory heuristic to reduce abusive traffic. It is
	 * not a persistent firewall and resets periodically.
	 */
	bool isBanned(string IP){
		cleanOld(); /* Fixes by Guilherme Gomes */
		if ( configuration->getFail2Ban() == 0 || logs.find(IP) == logs.end()) {
			return false;
		} else {
			Log log=logs[IP];
			return log.errors > 20 * configuration->getFail2Ban() &&
				   log.errors > log.requests / 2;
		}
	}
	/**
	 * @brief Update per-IP and global request/error counters.
	 * @param IP Client IP address as a string.
	 * @param es Exit status reported by the child.
	 */
	void countRequest(string IP, ExitStatus es){
		if(es == neutral){
			return;
		}
		Log log;
		if(logs.find(IP) != logs.end())
			log=logs[IP];
		log.requests++;
		if( es != success ){
			log.errors++;
			statistics.errors++;
		}
		logs[IP]=log;
	}
	/**
	 * @brief Finalize accounting for a finished child.
	 * @param pid Child PID.
	 * @param es Exit status reported by the child.
	 */
	void processChildEnd(pid_t pid, ExitStatus es){
		if(children.find(pid) == children.end())
			Logger::log(LOG_ERR,"Child end, but pid not found");
		Child c=children[pid];
		children.erase(pid);
		countRequest(c.IP,es);
	}
	/**
	 * @brief Harvest finished children (non-blocking).
	 *
	 * Uses `waitpid(-1, ..., WNOHANG)` to reap any dead children and records their
	 * exit status for statistics and fail2ban heuristics.
	 */
	void harvest(){
		while(true){
			if(children.size()==0) return;
			int status;
			pid_t wret=waitpid(-1, &status, WNOHANG);
			if(wret == 0) return; //All Children running o no child
			if(wret == -1){
				Logger::log(LOG_ERR,"Server waitpid error");
				return;
			}
			statistics.requests++;
			if(WIFSIGNALED(status)){
				processChildEnd(wret,internalError);
			}else if(WIFEXITED(status)){
				processChildEnd(wret, static_cast<ExitStatus>(WEXITSTATUS(status)));
			}
		}
	}
	/**
	 * @brief Periodically clear per-IP logs to bound memory usage.
	 */
	void cleanOld(){
		//Simple algorithm remove old every 5 minutes
		static time_t nextClear=0;
		if(nextClear > time(NULL)) return;
		nextClear= time(NULL)+5*60; //next 5 minutes
		logs.clear();
		/*
		for(map<string,Log>::iterator it=logs.begin(); it != logs.end(); it++){
			Log l=it->second;
			l.request /=2;
			l.errors  /=3;
			it->second = l;
		}
		 */
	}
	/**
	 * @brief Accept a client connection and fork a handler process.
	 *
	 * The parent process:
	 * - Performs IP-based rejection (optional).
	 * - Stores child bookkeeping (IP + start time).
	 *
	 * The child process:
	 * - Wraps the accepted FD into either `Socket` or `SSLSocket`.
	 * - Calls `Jail::process()`.
	 * - Exits with an `ExitStatus` code.
	 */
	void launch(int listen, bool sec){
		struct sockaddr_in client;
		socklen_t slen = sizeof(client);
		this->actualSocket = ::accept(listen, (struct sockaddr *)&client, &slen);
		if (this->actualSocket < 0) {
			Logger::log(LOG_ERR,"accept() Error:%d  %m", this->actualSocket);
			return;
		}
		string IP;
		char dst[INET_ADDRSTRLEN];
		const char *d = inet_ntop(client.sin_family, &client.sin_addr, dst, INET_ADDRSTRLEN);
		if(d != NULL)
			IP=d;
		if (isBanned(IP)) {
			close(this->actualSocket);
			statistics.rejected++;
			countRequest(IP,internalError);
			Logger::log(LOG_ERR,"%s: Request rejected: IP banned", IP.c_str());
			return;
		}
		fcntl(this->actualSocket, FD_CLOEXEC); //Close socket on execve
		pid_t pid = fork();
		if (pid == 0) { //new process
			Socket *socket = NULL;
			close(this->listenSocket);
			if(sec)
				socket= new SSLSocket(this->actualSocket);
			else
				socket= new Socket(this->actualSocket);
			Jail jail(IP);
			jail.process(socket);
			delete socket;
			usleep(10000);//Wait for father process child
			_exit(EXIT_SUCCESS);
		}
		close(this->actualSocket);
		Child c;
		c.IP=IP;
		c.start=time(NULL);
		children[pid]=c;	
	}

	/**
	 * @brief Poll listening sockets and fork handlers for ready connections.
	 *
	 * Uses `poll()` with timeout `JAIL_ACCEPT_WAIT`.
	 */
	void accept(){
		int res=poll(sockets, nSockets, JAIL_ACCEPT_WAIT);
		if ( res == -1 ) {
			if (errno == EINTR) {
				return;
			}
			throw string("Error poll sockets in accept: ") + strerror(errno);
		}
		if (res==0) return; //No request
		for ( int ns = 0; ns < nSockets; ns++ ) {
			if (sockets[ns].revents & POLLIN) { // Any type http/https or ws/wss
				launch(sockets[ns].fd, isSecureSocket[ns]);
			}
		}
	}
	/**
	 * @brief Create/bind/listen a TCP socket.
	 * @param port Port to bind.
	 * @param type Human readable label used for logging.
	 * @param timeout Retry window in seconds for bind() (useful during restarts).
	 * @return Listening socket fd, or -1 when port <= 0.
	 */
	int initSocket(int port, const char *type, int timeout=10) {
		if (port <= 0) {
			Logger::log(LOG_INFO,"No %s used", type);
			return -1;
		}
		struct sockaddr_in local;
		memset(&local, 0, sizeof(local));
		local.sin_family = AF_INET;
		if(this->interface > "")
			local.sin_addr.s_addr = inet_addr(interface.c_str());
		else
			local.sin_addr.s_addr = INADDR_ANY;
		int socketfd = socket(AF_INET, SOCK_STREAM, 0);
		if (socketfd == -1)
			throw "socket() error";
		#ifdef SO_REUSEPORT
		int on=1;
		if (setsockopt(socketfd, SOL_SOCKET, SO_REUSEPORT, &on, sizeof(on)) < 0) {
			Logger::log(LOG_ERR,"setsockopt(SO_REUSEPORT) %s failed: %m", type);
		}
		#endif
		local.sin_port = htons(port);
		int bindResult = -1;
		for (int i = 1; bindResult == -1 && i <= timeout; i++) {
			bindResult = bind(socketfd, (struct sockaddr *) &local, sizeof(local));
			if (bindResult == -1) {
				sleep(1);
				Logger::log(LOG_DEBUG,"bind() to %s retry %d: %m", type, i);
			}
		}
		if ( bindResult == -1)
			throw string("bind() to ") + type + " error: " + strerror(errno);
		if (listen(socketfd, 100) == -1) // queue size = 100
			throw "listen() error";
		Logger::log(LOG_INFO,"Listening at %s %s:%d", type, inet_ntoa(local.sin_addr), port);
		return socketfd;
	}

	/**
	 * @brief Initialize configured listening sockets.
	 *
	 * Creates up to two sockets: HTTP (`PORT`) and HTTPS (`SECURE_PORT`).
	 */
	void initSocketServer(){
		this->port = configuration->getPort();
		this->sport = configuration->getSecurePort();
		this->interface = configuration->getInterface();
		this->listenSocket = this->initSocket(this->port, "http plain port");
		this->secureListenSocket = this->initSocket(this->sport, "https secure port");
		this->nSockets = 0;

		if (this->listenSocket >= 0) {
			sockets[this->nSockets].fd = this->listenSocket;
			sockets[this->nSockets].events = POLLIN;
			isSecureSocket[this->nSockets] = false;
			this->nSockets++;
		}
		if (this->secureListenSocket >= 0) {
			sockets[this->nSockets].fd = this->secureListenSocket;
			sockets[this->nSockets].events = POLLIN;
			isSecureSocket[this->nSockets] = true;
			this->nSockets++;
		}
		if ( this->nSockets == 0) {
			Logger::log(LOG_EMERG, "No PORT or SECURE_PORT defined");
			_exit(EXIT_FAILURE);
		}
		actualSocket = -1;
	}

	/**
	 * @brief Construct and initialize the server singleton.
	 *
	 * Performs integrity checks, initializes SSL, and binds sockets.
	 */
	Daemon(){
		signal(SIGPIPE, SIG_IGN);
		signal(SIGTERM, SIGTERMHandler);
		this->listenSocket = -1;
		this->secureListenSocket=-1;
		configuration=Configuration::getConfiguration();
		checkJail();
		checkControlDir();
		SSLBase::getSSLBase();
		initSocketServer();
	}
public:
	static Daemon* getRunner(){
		if(singlenton == NULL) singlenton= new Daemon();
		return singlenton;
	}

	/**
	 * @brief Detach from controlling terminal (classic double-fork).
	 *
	 * Writes a PID file to `/run/vpl-jail-server.pid`.
	 */
	void daemonize(){
		pid_t child_pid = fork();
		if(child_pid < 0) {
			Logger::log(LOG_EMERG, "daemonize() => fork() fail (child_pid < 0)");
			exit(EXIT_FAILURE);
		}
		if(child_pid > 0) _exit(EXIT_SUCCESS); //gradparent exit
		if(setsid() < 0) {
			Logger::log(LOG_EMERG, "daemonize() => (setsid() < 0)");
			exit(EXIT_FAILURE);
		}
		pid_t grandchild_pid = fork();
		if(grandchild_pid < 0) {
			Logger::log(LOG_EMERG, "daemonize() => fork() fail (grandchild_pid < 0)");
			exit(EXIT_FAILURE);
		}
		if(grandchild_pid > 0) _exit(EXIT_SUCCESS); //parent exit
		FILE *fd=fopen("/run/vpl-jail-server.pid", "w");
		fprintf(fd, "%d", (int)getpid());
		fclose(fd);
	}

	/**
	 * @brief Run in the foreground but still create a PID file.
	 *
	 * In some environments (e.g., Docker) `setsid()` may fail; this is tolerated.
	 */
	void foreground(){
		setsid(); // NOTE: fail in Docker.
		FILE *fd=fopen("/run/vpl-jail-server.pid", "w");
		fprintf(fd, "%d", (int)getpid());
		fclose(fd);
	}

	/**
	 * @brief Close all server sockets
	 *
	 * Called at shutdown and when forking a child prisoner
	 */
	static void closeSockets(){
		Daemon* runner = getRunner();
		runner->nSockets = 0;
		if(runner->listenSocket > 0){
			close(runner->listenSocket);
			runner->listenSocket = -1;
		}
		if(runner->secureListenSocket > 0){
			close(runner->secureListenSocket);
			runner->secureListenSocket = -1;
		}
		if(runner->actualSocket > 0){
			close(runner->actualSocket);
			runner->actualSocket = -1;
		}
	}

	/**
	 * @brief Periodic maintenance tasks.
	 *
	 * Currently:
	 * - Refresh SSL context (best-effort)
	 * - Fork a helper to clean zombie tasks (best-effort)
	 */
	void periodicTasks() {
		static int checkPoint = 5 * 60 * 1000 / JAIL_ACCEPT_WAIT; // 5 minutes.
		static int loops = 0;
		loops++;
		if ( loops >= checkPoint ) {
			loops = 0;
			try {
				SSLBase::getSSLBase()->createUpdateContext();
			} catch(...) {
			}
			int pid = fork();
			if (pid == 0) {
				try {
					processMonitor::cleanZombieTasks();
				} catch(...) {
				}
				_exit(EXIT_SUCCESS);
			}
			Child c;
			c.IP = "127.0.0.1";
			c.start = time(NULL);
			children[pid]=c;
		}
	}
	/**
	 * @brief Main server loop.
	 *
	 * Repeats:
	 * - accept new connections (with a short timeout)
	 * - harvest finished children
	 * - run periodic maintenance
	 *
	 * Exits when SIGTERM sets `finishRequested`.
	 */
	void loop() {
		while(!finishRequested) {
			accept(); //Accept one request waiting JAIL_ACCEPT_WAIT msec
			harvest(); //Process all dead childrens
			periodicTasks();
		}
		closeSockets();
	}
};
