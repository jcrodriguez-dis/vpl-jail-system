/**
 * Tests for the flock-based global and task locks.
 */
#include "basetest.h"
#include <cassert>
#include <cstdlib>
#include <poll.h>
#include <signal.h>
#include <stdexcept>
#include <string>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include "../src/lock.h"

using namespace std;

class LockTestConfiguration: public Configuration {
protected:
	LockTestConfiguration(): Configuration("./configfiles/empty.txt") {}
public:
	static void useControlPath(const string &controlPath) {
		assert(setenv("VPL_JAIL_CONTROLPATH", controlPath.c_str(), 1) == 0);
		static LockTestConfiguration instance;
		singlenton = &instance;
	}
};

class LockTest: public BaseTest {
	static void writeByte(int fileDescriptor) {
		char value = 'x';
		ssize_t result;
		do {
			result = write(fileDescriptor, &value, 1);
		} while (result < 0 && errno == EINTR);
		assert(result == 1);
	}

	static void readByte(int fileDescriptor) {
		char value;
		ssize_t result;
		do {
			result = read(fileDescriptor, &value, 1);
		} while (result < 0 && errno == EINTR);
		assert(result == 1);
	}

	static bool hasByte(int fileDescriptor, int timeoutMilliseconds) {
		struct pollfd descriptor;
		descriptor.fd = fileDescriptor;
		descriptor.events = POLLIN;
		descriptor.revents = 0;
		int result;
		do {
			result = poll(&descriptor, 1, timeoutMilliseconds);
		} while (result < 0 && errno == EINTR);
		if (result <= 0 || !(descriptor.revents & (POLLIN | POLLHUP))) {
			return false;
		}
		char value;
		return read(fileDescriptor, &value, 1) == 1;
	}

	static void waitSuccessfully(pid_t child) {
		int status;
		assert(waitpid(child, &status, 0) == child);
		assert(WIFEXITED(status));
		assert(WEXITSTATUS(status) == EXIT_SUCCESS);
	}

	static string createControlPath() {
		char pathTemplate[] = "/tmp/vpl-lock-test-XXXXXX";
		char *path = mkdtemp(pathTemplate);
		assert(path != NULL);
		string controlPath(path);
		LockTestConfiguration::useControlPath(controlPath);
		return controlPath;
	}

	static void removeLockFiles(const string &controlPath) {
		const string lockPath = controlPath + "/locks/";
		const char *lockNames[] = {
			"global.lock", "task_123.lock", "task_456.lock", "valid.lock",
			"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa.lock"
		};
		for (size_t i = 0; i < sizeof(lockNames) / sizeof(lockNames[0]); i++) {
			unlink((lockPath + lockNames[i]).c_str());
		}
		rmdir((lockPath).c_str());
		rmdir(controlPath.c_str());
	}

	static bool rejects(const string &name) {
		try {
			Lock lock(name);
		} catch (const runtime_error &) {
			return true;
		}
		return false;
	}

	static void testNameValidation() {
		string maximumName(32, 'a');
		{
			Lock lock("valid");
		}
		{
			Lock lock(maximumName);
		}
		assert(rejects("") );
		assert(rejects(string(33, 'a')));
		assert(rejects("contains-dash"));
		assert(rejects("contains/slash"));
		assert(rejects("contains.dot"));
		assert(rejects(string(1, static_cast<char>(0x80))));
	}

	static void testGlobalAndTaskLocksAreIndependent() {
		int acquired[2];
		assert(pipe(acquired) == 0);
		GlobalLock globalLock;
		pid_t child = fork();
		assert(child >= 0);
		if (child == 0) {
			close(acquired[0]);
			try {
				TaskLock taskLock(123);
				writeByte(acquired[1]);
				close(acquired[1]);
				_exit(EXIT_SUCCESS);
			} catch (...) {
				_exit(EXIT_FAILURE);
			}
		}
		close(acquired[1]);
		assert(hasByte(acquired[0], 1000));
		close(acquired[0]);
		waitSuccessfully(child);
	}

	static void testGlobalLockExcludesAnotherProcess() {
		int ready[2];
		int start[2];
		int acquired[2];
		assert(pipe(ready) == 0);
		assert(pipe(start) == 0);
		assert(pipe(acquired) == 0);
		pid_t child = fork();
		assert(child >= 0);
		if (child == 0) {
			close(ready[0]);
			close(start[1]);
			close(acquired[0]);
			writeByte(ready[1]);
			readByte(start[0]);
			try {
				GlobalLock globalLock;
				writeByte(acquired[1]);
				close(acquired[1]);
				_exit(EXIT_SUCCESS);
			} catch (...) {
				_exit(EXIT_FAILURE);
			}
		}
		close(ready[1]);
		close(start[0]);
		close(acquired[1]);
		readByte(ready[0]);
		{
			GlobalLock globalLock;
			writeByte(start[1]);
			assert(!hasByte(acquired[0], 200));
		}
		assert(hasByte(acquired[0], 1000));
		close(ready[0]);
		close(start[1]);
		close(acquired[0]);
		waitSuccessfully(child);
	}

	static void testLockReleasedAfterProcessIsKilled() {
		int ready[2];
		int acquired[2];
		assert(pipe(ready) == 0);
		assert(pipe(acquired) == 0);
		pid_t holder = fork();
		assert(holder >= 0);
		if (holder == 0) {
			close(ready[0]);
			close(acquired[0]);
			try {
				GlobalLock globalLock;
				writeByte(ready[1]);
				for (;;) pause();
			} catch (...) {
				_exit(EXIT_FAILURE);
			}
		}
		close(ready[1]);
		readByte(ready[0]);

		pid_t waiter = fork();
		assert(waiter >= 0);
		if (waiter == 0) {
			close(ready[0]);
			close(acquired[0]);
			try {
				GlobalLock globalLock;
				writeByte(acquired[1]);
				close(acquired[1]);
				_exit(EXIT_SUCCESS);
			} catch (...) {
				_exit(EXIT_FAILURE);
			}
		}
		close(acquired[1]);
		assert(!hasByte(acquired[0], 200));
		assert(kill(holder, SIGKILL) == 0);
		int holderStatus;
		assert(waitpid(holder, &holderStatus, 0) == holder);
		assert(WIFSIGNALED(holderStatus));
		assert(hasByte(acquired[0], 1000));
		close(ready[0]);
		close(acquired[0]);
		waitSuccessfully(waiter);
	}

public:
	string name() {
		return "flock global and task locks";
	}

	void launch() {
		string controlPath = createControlPath();
		testNameValidation();
		testGlobalAndTaskLocksAreIndependent();
		testGlobalLockExcludesAnotherProcess();
		testLockReleasedAfterProcessIsKilled();
		removeLockFiles(controlPath);
	}
};

LockTest lockTest;
