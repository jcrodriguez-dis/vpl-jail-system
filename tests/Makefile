# Generate the tests programs
COMPILER = g++
CXXFLAGS = -std=c++11 -DHAVE_CONFIG_H -I.. 
LIBS = -lssl -lcrypto -lutil


all: program-test websocket-echo-test 


program-test: test.cpp  ../src/configurationFile.cpp ../src/configuration.cpp\
	../src/socket.cpp ../src/socket.h\
	../src/cgroup.cpp ../src/cgroup.h\
	../src/websocket.cpp ../src/websocket.h\
	../src/util.cpp ../src/util.h ../src/rpc.h ../src/xml.h ../src/jail_limits.h ../src/lock.h
	$(COMPILER) $(CXXFLAGS) -o program-test  test.cpp  ../src/configurationFile.cpp ../src/configuration.cpp \
		../src/socket.cpp ../src/cgroup.cpp ../src/websocket.cpp ../src/util.cpp $(LIBS) 
	
websocket-echo-test: webSocketEchoTest.cpp ../src/socket.cpp ../src/socket.h \
	../src/websocket.cpp ../src/websocket.h \
	../src/util.cpp ../src/util.h ../src/rpc.h ../src/xml.h ../src/jail_limits.h ../src/lock.h
	$(COMPILER) $(CXXFLAGS) -o websocket-echo-test  webSocketEchoTest.cpp ../src/configurationFile.cpp ../src/configuration.cpp \
		../src/socket.cpp ../src/websocket.cpp ../src/util.cpp $(LIBS) 
