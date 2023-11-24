# Generate the tests programs
COMPILER = g++
CXXFLAGS = -g -O0 -std=c++11 -DHAVE_CONFIG_H -I..
LIBS = -lssl -lcrypto -lutil
TESTCPPSRC = ../src/*.cpp
TESTHSRC = ../src/*.h

all: program-test websocket-echo-test


program-test: *test.cpp $(TESTCPPSRC) $(TESTHSRC)
	$(COMPILER) $(CXXFLAGS) -DTEST -o program-test  *test.cpp $(TESTCPPSRC) $(LIBS) 
	
websocket-echo-test: webSocketEchoTest.cpp ../src/socket.cpp ../src/socket.h \
	../src/websocket.cpp ../src/websocket.h \
	../src/util.cpp ../src/util.h ../src/rpc.h ../src/xml.h ../src/jail_limits.h ../src/lock.h
	$(COMPILER) $(CXXFLAGS) -o websocket-echo-test  webSocketEchoTest.cpp ../src/configurationFile.cpp ../src/configuration.cpp \
		../src/socket.cpp ../src/websocket.cpp ../src/util.cpp ../src/log.cpp $(LIBS) 
