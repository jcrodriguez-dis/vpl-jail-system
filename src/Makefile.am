bin_PROGRAMS = vpl-jail-server
vpl_jail_server_SOURCES = main.cpp vpl-jail-server.cpp vpl-jail-server.h jail.cpp jail.h\
httpServer.cpp httpServer.h httpException.h\
redirector.cpp redirector.h\
configurationFile.cpp configurationFile.h\
configuration.cpp configuration.h\
processMonitor.cpp processMonitor.h\
socket.cpp socket.h\
websocket.cpp websocket.h\
util.cpp util.h requestmessage.h rpc.h xmlrpc.h xml.h xml.cpp jsonrpc.h json.h json.cpp jail_limits.h lock.h\
cgroup.cpp cgroup.h vplregex.h\
log.cpp log.h

# set the include path found by configure
#INCLUDES= $(all_includes)

# the library search path.
#AM_LDFLAGS = $(all_libraries)
# Detected C++17 or higher
if CPP17
AM_CXXFLAGS=
else
# Minimum C++ version
AM_CXXFLAGS= -std=c++11
endif