#include <assert.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

#include <string>

#include "../src/configuration.h"
#include "../src/socket.h"
#include "../src/websocket.h"

using namespace std;

class ConfigurationTestFull: public Configuration {
protected:
	ConfigurationTestFull(): Configuration("tests/configfiles/full.txt") {}
public:
	static Configuration* getConfiguration(){
		static ConfigurationTestFull instance;
		singlenton = &instance;
		return singlenton;
	}
};

static void writeAll(int fd, const string &data) {
	size_t offset = 0;
	while (offset < data.size()) {
		ssize_t written = write(fd, data.data() + offset, data.size() - offset);
		assert(written > 0);
		offset += written;
	}
}

static string readAll(int fd, size_t size) {
	string data(size, '\0');
	size_t offset = 0;
	while (offset < size) {
		ssize_t readSize = read(fd, &data[offset], size - offset);
		assert(readSize > 0);
		offset += readSize;
	}
	return data;
}

static void createTCPPair(int descriptors[2]) {
	int listener = socket(AF_INET, SOCK_STREAM, 0);
	assert(listener >= 0);
	struct sockaddr_in address;
	memset(&address, 0, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	address.sin_port = 0;
	assert(bind(listener, (struct sockaddr *)&address, sizeof(address)) == 0);
	assert(listen(listener, 1) == 0);
	socklen_t addressSize = sizeof(address);
	assert(getsockname(listener, (struct sockaddr *)&address, &addressSize) == 0);
	descriptors[1] = socket(AF_INET, SOCK_STREAM, 0);
	assert(descriptors[1] >= 0);
	assert(connect(descriptors[1], (struct sockaddr *)&address, sizeof(address)) == 0);
	descriptors[0] = accept(listener, NULL, NULL);
	assert(descriptors[0] >= 0);
	close(listener);
	int bufferSize = 1024 * 1024;
	assert(setsockopt(descriptors[0], SOL_SOCKET, SO_SNDBUF, &bufferSize, sizeof(bufferSize)) == 0);
	assert(setsockopt(descriptors[1], SOL_SOCKET, SO_RCVBUF, &bufferSize, sizeof(bufferSize)) == 0);
}

static void consumeHandshake(int fd) {
	string response;
	char byte;
	while (response.find("\r\n\r\n") == string::npos) {
		assert(read(fd, &byte, 1) == 1);
		response += byte;
	}
	assert(response.find("HTTP/1.1 101 Switching Protocols") == 0);
}

static string maskedFrame(FrameType type, const string &payload) {
	string frame;
	frame += static_cast<char>(0x80 | type);
	if (payload.size() <= 125) {
		frame += static_cast<char>(0x80 | payload.size());
	} else if (payload.size() <= 0xffff) {
		frame += static_cast<char>(0x80 | 126);
		frame += static_cast<char>(payload.size() >> 8);
		frame += static_cast<char>(payload.size() & 0xff);
	} else {
		frame += static_cast<char>(0x80 | 127);
		for (int shift = 56; shift >= 0; shift -= 8) {
			frame += static_cast<char>(payload.size() >> shift);
		}
	}
	const char mask[] = { 0x12, 0x34, 0x56, 0x78 };
	frame.append(mask, sizeof(mask));
	for (size_t index = 0; index < payload.size(); ++index) {
		frame += static_cast<char>(payload[index] ^ mask[index % sizeof(mask)]);
	}
	return frame;
}

static void assertOutboundFrame(int fd, FrameType type, const string &payload) {
	string header = readAll(fd, 2);
	assert(static_cast<unsigned char>(header[0]) == static_cast<unsigned char>(0x80 | type));
	assert((static_cast<unsigned char>(header[1]) & 0x80) == 0);
	size_t payloadSize = static_cast<unsigned char>(header[1]) & 0x7f;
	if (payloadSize == 126) {
		header += readAll(fd, 2);
		payloadSize = (static_cast<unsigned char>(header[2]) << 8) |
			static_cast<unsigned char>(header[3]);
	} else if (payloadSize == 127) {
		header += readAll(fd, 8);
		payloadSize = 0;
		for (size_t index = 2; index < header.size(); ++index) {
			payloadSize = (payloadSize << 8) | static_cast<unsigned char>(header[index]);
		}
	}
	assert(payloadSize == payload.size());
	assert(readAll(fd, payloadSize) == payload);
}

static void assertOutboundFrameAsync(webSocket &websocket, int fd, FrameType type, const string &payload) {
	pid_t pid = fork();
	assert(pid >= 0);
	if (pid == 0) {
		assertOutboundFrame(fd, type, payload);
		_exit(EXIT_SUCCESS);
	}
	websocket.send(payload, type);
	int status;
	assert(waitpid(pid, &status, 0) == pid);
	assert(WIFEXITED(status) && WEXITSTATUS(status) == EXIT_SUCCESS);
}

int main() {
	try {
	ConfigurationTestFull::getConfiguration();
	int descriptors[2];
	createTCPPair(descriptors);
	Socket socket(descriptors[0]);
	writeAll(descriptors[1], "GET /test HTTP/1.1\r\nSec-WebSocket-Key: key\r\n\r\n");
	socket.readHeaders();
	webSocket websocket(&socket);
	consumeHandshake(descriptors[1]);

	const string payload125(125, 'a');
	const string payload126(126, 'b');
	const string payload65535(65535, 'c');
	const string payload65536(65536, 'd');
	websocket.send(payload125);
	assertOutboundFrame(descriptors[1], TEXT_FRAME, payload125);
	websocket.send(payload126);
	assertOutboundFrame(descriptors[1], TEXT_FRAME, payload126);
	websocket.send(payload65535);
	assertOutboundFrame(descriptors[1], TEXT_FRAME, payload65535);
	assertOutboundFrameAsync(websocket, descriptors[1], TEXT_FRAME, payload65536);

	writeAll(descriptors[1], maskedFrame(TEXT_FRAME, payload126));
	assert(websocket.receive() == payload126);
	writeAll(descriptors[1], maskedFrame(PING_FRAME, "ping"));
	assert(websocket.receive().empty());
	assertOutboundFrame(descriptors[1], PONG_FRAME, "ping");

	websocket.close("Bye");
	string closePayload;
	closePayload += static_cast<char>(1000 >> 8);
	closePayload += static_cast<char>(1000 & 0xff);
	closePayload += "Bye";
	assertOutboundFrame(descriptors[1], CONNECTION_CLOSE_FRAME, closePayload);
	close(descriptors[1]);
	return 0;
	} catch (HttpException &exception) {
	fprintf(stderr, "Unexpected HttpException: %s\n", exception.getLog().c_str());
	return 1;
	}
}