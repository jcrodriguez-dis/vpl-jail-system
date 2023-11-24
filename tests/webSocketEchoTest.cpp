#include <string>
#include <iostream>

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <syslog.h>
using namespace std;

#include "../src/websocket.h"
#include "../src/httpServer.h"

class ConfigurationTestFull: public Configuration {
protected:
	ConfigurationTestFull(): Configuration("./configfiles/full.txt") {}
public:
	static Configuration* getConfiguration(){
		singlenton = new ConfigurationTestFull();
		return singlenton;
	}
};

int main() {
	ConfigurationTestFull::getConfiguration();
	openlog("websocket-echo",LOG_PID,LOG_USER);
	setlogmask(LOG_UPTO(LOG_DEBUG));
	Logger::log(LOG_INFO,"Websocket echo test program started");
	int listenSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (listenSocket == -1) {
		cerr << "Socket error" << endl;
		exit(0);
	}

	struct sockaddr_in local;
	memset(&local, 0, sizeof(local));
	local.sin_family = AF_INET;
	local.sin_addr.s_addr = INADDR_ANY;
	local.sin_port = htons(8080);
	if (bind(listenSocket, (struct sockaddr *) &local, sizeof(local)) == -1) {
		cerr << "bind failed" << endl;
		exit(0);
	}

	if (listen(listenSocket, 1) == -1) {
		cerr << "listen failed" << endl;
		exit(0);
	}
	// cout << "opened " << inet_ntoa(local.sin_addr) << ":" << ntohs(local.sin_port) << endl;

	while (1) {
		struct sockaddr_in remote;
		socklen_t sockaddrLen = sizeof(remote);
		int clientSocket = accept(listenSocket, (struct sockaddr*)&remote, &sockaddrLen);
		if (clientSocket == -1) {
			cerr << "accept failed" << endl;
			exit(0);
		}
		try{
			// cout << "connected " << inet_ntoa(remote.sin_addr) << ":" << ntohs(remote.sin_port) <<endl;
			Socket socket(clientSocket);
			socket.readHeaders();
			webSocket ws(&socket);
			ws.send("Hello from echo websocket", TEXT_FRAME);
			while(!ws.isClosed()){
				while(ws.wait());
				string r=ws.receive();
				if(r == "close") {
					ws.send("disconnecting",TEXT_FRAME);
					ws.close();
					break;
				}
				if(r == "end") {
					ws.send("finished",TEXT_FRAME);
					ws.close();
					close(listenSocket);
					return EXIT_SUCCESS;
				}
				if(r.size()>0) {
					ws.send(r, ws.lastFrameType());
				}
			}
			// cout << "disconnected" << endl;
		} catch(HttpException &e) {
			cerr << "HttpException: " << e.getLog() << endl;
		} catch(const std::exception &e){
			cerr << "Unexpected exception:" << e.what() << " " << __FILE__ << ":" << __LINE__;
		}catch(const string &e) {
			cerr << "Exception: " << e << endl;
		} catch(const char *e) {
			cerr << "Exception: " << e << endl;
		} catch(...) {
			cerr << "Unknow exception" << endl;
		}

	}
	return EXIT_SUCCESS;
}



