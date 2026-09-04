/**
 * version:		$Id: util.h,v 1.23 2014/12/19 13:00:28 juanca Exp $
 * package:		Part of vpl-jail-system
 * copyright:	Copyright (C) 2009 Juan Carlos Rodríguez-del-Pino. All rights reserved.
 * license:		GNU/GPL, see LICENSE.txt or http://www.gnu.org/licenses/gpl-3.0.html
 **/

#ifndef VPL_UTIL_INC_H
#define VPL_UTIL_INC_H
#if HAVE_CONFIG_H
#include <config.h>
#endif
#include <limits>
#include <map>
#include <iostream>
#include <fstream>
#include <sstream>
#ifdef HAVE_WEAKLY_CANONICAL
#include <filesystem>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <fcntl.h>
#include <syslog.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <signal.h>
#include <string.h>
#include <string>
#include "log.h"
#include "vplregex.h"
#include "jail_limits.h"
#include "httpException.h"

#if HAVE_LINUX_OPENAT2_H
#include <linux/openat2.h>
#ifndef SYS_openat2
#define SYS_openat2 437
#endif
#endif

#define VPL_EXECUTION "vpl_execution"
#define VPL_WEXECUTION "vpl_wexecution"
#define VPL_WEBEXECUTION "vpl_webexecution"
#define VPL_WEBCOOKIE "VPL_web"
#define VPL_SETWEBCOOKIE "Set-Cookie: " VPL_WEBCOOKIE "="
#define VPL_CLEANWEBCOOKIE VPL_SETWEBCOOKIE "n; Max-Age=-1\r\n"
#define VPL_IWASHERECOOKIE "VPL_Iwh"
#define VPL_SETIWASHERECOOKIE "Set-Cookie: " VPL_IWASHERECOOKIE "=y; Path=/; SameSite=none; Secure; Partitioned\r\n"
#define VPL_LOCALREDIRECT "Location: /\r\n"
#define VPL_LOCALSERVERADDRESSFILE ".vpl_localserveraddress"

using namespace std;

enum ExitStatus {
	success=EXIT_SUCCESS,
	internalError=EXIT_FAILURE,
	neutral,
	httpError,
	websocketError
};

struct ExecutionLimits {
	int maxtime;
	long long maxfilesize;
	long long maxmemory;
	int maxprocesses;
	void log(const char *s){
		Logger::log(LOG_DEBUG, "%s: maxtime: %d sec, maxfilesize: %lld Kb, maxmemory %lld Kb, maxprocesses: %d",
				s, maxtime, maxfilesize/1024, maxmemory/1024, maxprocesses);
	}
};

class Base64{
	static int C642int[256];
	void init(){
		static const char *charSet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
		int l=strlen(charSet);
		for(int i=0;i<256;i++)
			C642int[i]=-1;
		for (int i = 0; i < l; i++)
			C642int[ (int) charSet[i]] = i<<2;
	}
	static bool is64(char c){
		int v=*((unsigned char *)&c);
		return C642int[v]>=0;
	}

	static int decodeChar(char c){
		int v=*((unsigned char *)&c);
		return C642int[v];
	}

	static int decodeSize(string data){
		int l=data.size();
		int s=0;
		int eque=0;
		for(int i=0;i<l;i++){
			char c=data[i];
			if(is64(c)) s++;
			else if(c=='=') eque++;
		}
		int size=(s+eque)/4*3-eque;
		return size;
	}

	static void setData(string &data,int pos,char c){
		int ibyte = pos/8;
		int ides = pos%8;
		int dc = decodeChar(c);
		int size = (int) data.size();
		if(ibyte >= size)return;
		//Sorry for to be so raw
		unsigned char *rawdata = (unsigned char *)data.c_str();
		rawdata[ibyte] |= (unsigned char)(dc>>ides);
		if(ides>2 && ibyte+1 < size){
			rawdata[ibyte+1] |= (unsigned char)(dc<<(8-ides));
		}
	}
public:
	Base64(){
		init();
	}
	static string decode(string data){
		int size=decodeSize(data);
		if(size==0) return "";
		string res(size,'\0');
		int l=data.size();
		int pos=0;
		for(int i=0; i<l;i++){
			char c=data[i];
			if(is64(c)){
				setData(res, pos, c);
				pos+=6;
			}
		}
		return res;
	}

	static string encode(const string &data){
		static const char *charSet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
		const unsigned char *rawdata=(const unsigned char *)data.data();
		int in_size=data.size();
		int out_size=((in_size+2)/3)*4;
		string ret(out_size,'\0');
		for(int out=0,in=0; out<out_size;out++){
			int in1=in<in_size?rawdata[in++]:0;
			int in2=in<in_size?rawdata[in++]:0;
			int in3=in<in_size?rawdata[in++]:0;
			ret[out++]=charSet[(in1>>2)&0x3f];
			ret[out++]=charSet[((in1&0x3)<<4)+(in2>>4)];
			ret[out++]=charSet[((in2&0xf)<<2)+(in3>>6)];
			ret[out  ]=charSet[in3&0x3f];
		}
		if(in_size%3 == 2) ret[out_size-1]='=';
		if(in_size%3 == 1){
			ret[out_size-2]='=';
			ret[out_size-1]='=';
		}
		return ret;
	}
};

/**
 * class for util procedure/functions
 */
class Util{
public:
	/**
	 * return the next line (LF ended)
	 * @param s string with lines
	 * @param offset the line start (updated to the next line start)
	 */
	static string getLine(const string &s, size_t &offset){
		if(s.size()<=offset) return "";
		size_t boffset=offset;
		while(s.size()>offset && s[offset]!='\n')
			offset++;
		size_t eoffset=offset;
		if(s.size()==eoffset){
			return s.substr(boffset,eoffset-boffset);
		}
		//s[offset]=='\n'
		offset++;
		if(eoffset>boffset && s[eoffset-1]=='\r')
			eoffset--;
		return s.substr(boffset,eoffset-boffset);
	}

	/**
	 * Returns a random int taken from /dev/urandom + std::rand()
	 */
	static int random() {
		static int randomFile = -1;
		if (randomFile == -1) {
			randomFile = open("/dev/urandom", O_RDONLY); // May need a close?
		}
		int ret = 13131313;
		if (randomFile > 0) {
			size_t r = read(randomFile, ((void*) (&ret)), (sizeof(int)));
			if (r != sizeof(int)) {
				ret = std::rand();
			}
		} else {
			ret = std::rand();
		}
		return abs(ret + std::rand());
	}

	/**
	 * Sleep for the specified number of microseconds
	 * Replaces deprecated usleep
	 */
	static void sleep(long microseconds);

	/**
	 * Compare two strings in constant time to prevent timing attacks
	 * @param a first string
	 * @param b second string
	 * @return true if both strings are equal, false otherwise
	 */
	static bool compareConstantTime(const string &a, const string &b) {
		size_t workSize = a.size() > b.size() ? a.size() : b.size();
		if (workSize % 32 != 0) workSize += 32 - workSize % 32;
		volatile unsigned char result = 0;
		for (size_t i = 0; i < workSize; i++) {
			unsigned char aByte = i < a.size() ? a[i] : 0;
			unsigned char bByte = i < b.size() ? b[i] : 0;
			result |= aByte ^ bByte;
		}
		result |= a.size() != b.size();
		return result == 0;
	}

	/**
	 * Cleans the string removing spaces from end and start
	 * remove a 'text'=>text and "text"=>text
	 */
	static void trimAndRemoveQuotes(string &s){
		while(s.size()>0 && s[0] == ' ') s.erase(0,1);
		while(s.size()>0 && s[s.size()-1] == ' ') s.erase(s.size()-1,1);
		if(s.size()>1 && s[0]=='\'' && s[s.size()-1] == '\''){ //remove ''' surrounding
			s.erase(s.size()-1,1);
			s.erase(0,1);
			return;
		}
		if(s.size()>1 && s[0]=='"' && s[s.size()-1] == '"'){ //remove '"' surrounding
			s.erase(s.size()-1,1);
			s.erase(0,1);
		}
	}

	/**
	 * Get process name and executable path from pid
	 * @param pid process id
	 * @param pname process name
	 * @param exe_path executable path
	 */
	static void getProcessName(pid_t pid, string &pname, string &exe_path){
		char path[PATH_MAX] = "";
		string p = "/proc/" + itos(pid) + "/exe";
		ssize_t len = readlink(p.c_str(), path, sizeof(path) - 1);
		if (len != -1) {
			path[len] = '\0';
		} else {
			path[0] = '\0';
		}
		exe_path = path;
		string fname = "/proc/" + itos(pid) + "/comm";
		pname = readFile(fname, false);
	}

	/**
	 * Check if process exists
	 */
	static bool processExists(pid_t pid){
		return pid > 0 && kill(pid, 0) == 0;
	}

	/**
	 * Check if file exists
	 */
	static bool fileExists(const string &fileName, bool followLink=false){
		struct stat info;
		if (followLink) {
			return stat(fileName.c_str(),&info)==0
					&& S_ISREG(info.st_mode);
		} else {
			return lstat(fileName.c_str(),&info)==0
					&& S_ISREG(info.st_mode);
		}
	}

	/**
	 * Check if directory exists, no symbolic links
	 */
	static bool dirExists(const string &fileName){
		struct stat info;
		int res = lstat(fileName.c_str(), &info);
		return res == 0 && S_ISDIR(info.st_mode);
	}

	/**
	 * Check if directory exists, follow symbolic links
	 */
	static bool dirExistsFollowingSymLink(const string &fileName){
		struct stat info;
		int res = stat(fileName.c_str(),&info);
		return res==0 && S_ISDIR(info.st_mode);
	}

	/**
	 * return a double as string
	 */
	static string dtos(double value){
		const int maxIntChars = 60;
		char buf[maxIntChars];
		sprintf(buf,"%lg",value);
		return buf;
	}

	/**
	 * return an int/long int as string
	 */
	static string itos(const long long value){
		const int maxIntChars = 31;
		char buf[maxIntChars];
		sprintf(buf,"%lld",value);
		return buf;
	}

	/**
	 * return an string as int
	 */
	static int atoi(const string &s){
		long int longValue = atol(s);
		if ( longValue >  numeric_limits<int>::max()) {
			return numeric_limits<int>::max();
		}
		return longValue;
	}

	/**
	 * return an string as long long
	 */
	static long long atol(const string &s){
		return ::atoll(s.c_str());
	}

	/**
	 * return the value of Kb, Mb or Gb
	 */
	static long long memAbbreviation(const string &abbreviation){
		const long long kb = 1024;
		const long long mb = 1024 * kb;
		const long long gb = 1024 * mb;
		if (abbreviation.size() == 0) {
			return 1;
		}
		char abb = abbreviation.at(0);
		if ( abb == 'K' || abb == 'k' ) {
			return kb;
		} else if (abb == 'M' || abb == 'm') {
			return mb;
		} else if (abb == 'G' || abb == 'g') {
			return gb;
		}
		return 1;
	}

	/**
	 * return a memory size in Gb, Mb or Kb to as bytes int
	 */
	static int memSizeToBytesi(const string &s){
		long long value = memSizeToBytesl(s);
		if ( value >  numeric_limits<int>::max() ||
		     value <= 0 ) {
			return numeric_limits<int>::max();
		}
		return value;
	}

	static const vplregex regMemSize;

	/**
	 * return a memory size in Gb, Mb or Kb to as bytes long long
	 */
	static long long memSizeToBytesl(const string &memSize){
		const int numberGroup = 1;
		const int abbrebiationGroup = 2;
		vplregmatch found(3);
		bool matchFound = regMemSize.search(memSize, found);
		if (matchFound) {
			return atol(found[numberGroup]) * memAbbreviation(found[abbrebiationGroup]);
		} else {
			return 0;
		}
	}

	/**
	 * Fix memory size -1 due XML-RPC limits
	 */
	static long long fixMemSize(long long memSize){
		if (memSize <= 0) {
			return numeric_limits<long long>::max();
		} else {
			return memSize;
		}
	}

	/**
	 * return upper case string
	 */
	static string toUppercase(const string & s){
		string ret(s);
		for(size_t i=0; i<ret.size();i++)
			ret.at(i)=toupper( ret.at(i));
		return ret;
	}
	/**
	 * read command from line
	 * @param argc as main param
	 * @param argv as main param
	 * @param command command, example "-d"
	 * @param data param after command
	 */
	static string getCommand(const int argc, const char ** const argv, const string &command){
		for(int i=1; i< (argc-1); i++){
			if(string(argv[i]) == command){
				return argv[i+1];
			}
		}
		return "";
	}
	/**
	 * @return environment var value or ""
	 * @param env string var name
	 */
	static string getEnv(const string env) {
		const char *res = getenv(env.c_str());
		if (res == NULL) return "";
		return res;
	}

	/**
	 * @return environment var name value or ""
	 * @param env string var in raw format name=value
	 */
	static string getEnvNameFromRaw(const string env) {
		size_t pos = env.find("=");
		if (pos == string::npos) {
			return env;
		} else {
			return env.substr(0, pos);
		}
	}

	static const vplregex correctFileNameReg;

	static bool correctFileName(const string &fn){
		if (fn.size() < 1) {
			Logger::log(LOG_DEBUG, "incorrectFile size = 0");
			return false;
		}
		if (fn.size() > JAIL_FILENAME_SIZE_LIMIT) {
			Logger::log(LOG_DEBUG, "incorrectFile size > %d", JAIL_FILENAME_SIZE_LIMIT);
			return false;
		}
		for (size_t i = 0; i < fn.size(); i++) {
			if (fn[i] == '\0') {
				Logger::log(LOG_DEBUG, "incorrectFile containing 0 char code '%s'",
							fn.c_str());
				return false;
			}
		}
		vplregmatch found(1);
		bool incorrect = correctFileNameReg.search(fn, found);
		if (incorrect) {
			string incorrect = found[0];
			Logger::log(LOG_DEBUG,"incorrectFile '%s' found '%s'"
					,fn.c_str(), incorrect.c_str());
		}
		return ! incorrect;
	}

	static bool correctPath(const string &path){ //No trailing /
		if (path.size() == 0) {
			Logger::log(LOG_DEBUG, "file path size = 0");
			return false;
		}
		if (path.size() > JAIL_PATH_SIZE_LIMIT) {
			Logger::log(LOG_DEBUG, "file path size > %d", JAIL_PATH_SIZE_LIMIT);
			return false;
		}
		size_t pos = 0;
		size_t found;
		string fn;
		if (path[0] == '/') pos = 1; //skip absolute path
		if (path.size() > 1 && path[0] == '.' && path[1] == '/') pos = 2; //skip relative path
		if (path.size() <= pos) {
			Logger::log(LOG_DEBUG, "file path with no file '%s'", path.c_str());
			return false;
		}
		while((found = path.find('/', pos)) != string::npos){
			fn = path.substr(pos, found - pos);
			if (!correctFileName(fn)) return false;
			pos = found + 1;
		}
		fn = path.substr(pos);
		return correctFileName(fn);
	}

	/**
	 * Returns the directory path of a filepath
	 * @param filePath file path
	 * @return directory path or ""
	 */
	static string getDirectory(const string &filePath){
		size_t pos;
		if ((pos = filePath.rfind('/')) != string::npos) {
			return filePath.substr(0, pos);
		}
		return "";
	}

	/**
	 * Create a directory from position in path
	 * @param filePath file path
	 * @return file name or ""
	 */
	static bool createDir(const string &path, uid_t user, size_t pos = 1) { //absolute path
		pos = pos == 0 ? 1 : pos;
		Logger::log(LOG_DEBUG, "createDir '%s' user %d pos %u", path.c_str(), user, pos);
		// O_PATH is used to walk the path with execute permission only.
		int directoryFd = path[0] == '/' ? open("/", O_PATH | O_DIRECTORY) :
			open(".", O_PATH | O_DIRECTORY);
		if (directoryFd < 0) {
			Logger::log(LOG_DEBUG, "Can't open base directory for '%s': %s (errno=%d)",
					path.c_str(), strerror(errno), errno);
			return false;
		}

		size_t componentStart = path[0] == '/' ? 1 : 0;
		while (componentStart < path.size()) {
			size_t separator = path.find('/', componentStart);
			if (separator == string::npos) separator = path.size();
			string component = path.substr(componentStart, separator - componentStart);
			if (component.empty()) {
				componentStart = separator + 1;
				continue;
			}

			bool created = false;
			if (separator >= pos && mkdirat(directoryFd, component.c_str(), 0700) == 0) {
				created = true;
			} else if (separator >= pos && errno != EEXIST) {
				Logger::log(LOG_DEBUG, "Can't create dir '%s': %s (errno=%d)",
						path.c_str(), strerror(errno), errno);
				close(directoryFd);
				return false;
			}

			if (created && user
					&& fchownat(directoryFd, component.c_str(), user, user, AT_SYMLINK_NOFOLLOW)) {
				Logger::log(LOG_DEBUG, "Can't change owner of directory '%s': %s (errno=%d)",
						path.c_str(), strerror(errno), errno);
				close(directoryFd);
				return false;
			}

			// Symlinks are only rejected in the untrusted part of the path.
			int nextDirectoryFd = openat(directoryFd, component.c_str(),
				O_PATH | O_DIRECTORY | (separator >= pos ? O_NOFOLLOW : 0));
			if (nextDirectoryFd < 0) {
				Logger::log(LOG_DEBUG, "Can't open directory component '%s' of '%s': %s (errno=%d)",
						component.c_str(), path.c_str(), strerror(errno), errno);
				close(directoryFd);
				return false;
			}
			close(directoryFd);
			directoryFd = nextDirectoryFd;
			componentStart = separator + 1;
		}
		close(directoryFd);
		return true;
	}

	static void removeCRs(string &text) {
		size_t len = text.size();
		bool noNL = true;
		for(size_t i = 0; i < len; i++) {
			if (text[i] == '\n') {
				noNL = false;
				break;
			};
		}
		if (noNL) { //Replace CR by NL
			for(size_t i = 0; i < len; i++) {
				if (text[i] == '\r') {
					text[i] = '\n';
				}
			}
		} else { //Remove CRs if any
			size_t lenClean = 0;
			for(size_t i = 0; i < len; i++) {
				if (text[i] != '\r') {
					text[lenClean] = text[i];
					lenClean++;
				}
			}
			text.resize(lenClean);
		}
	}

	static bool pathChanged(const string& filePath, size_t pos) {
		if (pos) {
			size_t found;
			string dn;
			struct stat info;
			while((found = filePath.find('/', pos)) != string::npos) {
				if (filePath.substr(pos, found - pos) == "..") return true;
				dn = filePath.substr(0, found);
				if (lstat(dn.c_str(), &info) == 0) {
					if (S_ISLNK(info.st_mode)) return true;
				}
				pos = found + 1;
			}
			if (lstat(filePath.c_str(), &info) != 0) return false;
			if (S_ISLNK(info.st_mode)) return true;
		}
		return false;
	}

	/**
	 * Open a file rejecting symlinks in the path components after "pos".
	 * The path prefix before "pos" is trusted, so it may contain symlinks
	 * (e.g. /usr/sbin or /proc/self in some systems).
	 */
	static int openFileWithoutSymlinks(const string &name, int flags, mode_t mode = 0, size_t pos = 0) {
		string base = name.substr(0, pos);
		string relative = name.substr(base.size());
		while (relative.size() && relative[0] == '/') relative.erase(0, 1);
		while (base.size() > 1 && base[base.size() - 1] == '/') base.erase(base.size() - 1);
		if (pos == 0 || relative.empty()) { // The whole path is trusted.
			int trustedFd = open(name.c_str(), flags, mode);
			if (trustedFd < 0) {
				Logger::log(LOG_DEBUG, "Can't open trusted path '%s': %s (errno=%d)",
						name.c_str(), strerror(errno), errno);
			}
			return trustedFd;
		}
		if (base.empty()) base = name[0] == '/' ? "/" : ".";
		int baseFd = open(base.c_str(), O_PATH | O_DIRECTORY);
		if (baseFd < 0) {
			Logger::log(LOG_DEBUG, "Can't open base directory '%s' for '%s': %s (errno=%d)",
					base.c_str(), name.c_str(), strerror(errno), errno);
			return -1;
		}
		#if HAVE_LINUX_OPENAT2_H
		{
			struct open_how how = {};
			how.flags = flags;
			how.mode = mode;
			how.resolve = RESOLVE_NO_SYMLINKS;
			int openat2Fd = syscall(SYS_openat2, baseFd, relative.c_str(), &how, sizeof(how));
			if (openat2Fd >= 0) {
				close(baseFd);
				return openat2Fd;
			}
			Logger::log(LOG_DEBUG, "openat2 failed for '%s': %s (errno=%d); trying openat fallback",
					name.c_str(), strerror(errno), errno);
		}
		#endif

		string directory = getDirectory(relative);
		string fileName = directory.size() ? relative.substr(directory.size() + 1) : relative;
		int directoryFd = baseFd;
		size_t componentStart = 0;
		while (componentStart < directory.size()) {
			size_t separator = directory.find('/', componentStart);
			if (separator == string::npos) separator = directory.size();
			string component = directory.substr(componentStart, separator - componentStart);
			if (component.size()) {
				int nextDirectoryFd = openat(directoryFd, component.c_str(),
						O_PATH | O_DIRECTORY | O_NOFOLLOW);
				if (nextDirectoryFd < 0) {
					Logger::log(LOG_DEBUG, "Can't open fallback directory component '%s' for '%s': %s (errno=%d)",
							component.c_str(), name.c_str(), strerror(errno), errno);
					close(directoryFd);
					return -1;
				}
				close(directoryFd);
				directoryFd = nextDirectoryFd;
			}
			componentStart = separator + 1;
		}
		int fileFd = openat(directoryFd, fileName.c_str(), flags | O_NOFOLLOW, mode);
		if (fileFd < 0) {
			Logger::log(LOG_DEBUG, "openat: Can't open fallback file '%s': %s (errno=%d)",
					fileName.c_str(), strerror(errno), errno);
		}
		close(directoryFd);
		return fileFd;
	}

	static int removeDirContents(int directoryFd, uid_t owner, bool force) {
		struct stat directoryStat;
		if (fstat(directoryFd, &directoryStat) != 0 || !S_ISDIR(directoryStat.st_mode)) {
			close(directoryFd);
			return 0;
		}
		bool removeAll = force || directoryStat.st_uid == owner || directoryStat.st_gid == owner;
		DIR *directory = fdopendir(directoryFd);
		if (directory == NULL) {
			close(directoryFd);
			return 0;
		}
		int removed = 0;
		dirent *entry;
		while ((entry = readdir(directory)) != NULL) {
			string name(entry->d_name);
			if (name == "." || name == "..") continue;

			int childFd = openat(dirfd(directory), name.c_str(),
					O_RDONLY | O_DIRECTORY | O_NOFOLLOW);
			if (childFd >= 0) {
				struct stat childStat;
				if (fstat(childFd, &childStat) == 0 && S_ISDIR(childStat.st_mode)) {
					bool removeChild = removeAll || childStat.st_uid == owner || childStat.st_gid == owner;
					removed += removeDirContents(childFd, owner, removeAll);
					if (removeChild && unlinkat(dirfd(directory), name.c_str(), AT_REMOVEDIR) == 0)
						removed++;
				} else {
					close(childFd);
				}
				continue;
			}

			struct stat childStat;
			if (fstatat(dirfd(directory), name.c_str(), &childStat, AT_SYMLINK_NOFOLLOW) != 0)
				continue;
			bool remove = removeAll || childStat.st_uid == owner || childStat.st_gid == owner;
			if (remove && unlinkat(dirfd(directory), name.c_str(), 0) == 0)
				removed++;
		}
		closedir(directory);
		return removed;
	}

	/**
	 * Writes to a file, creating or replacing it if it already exists.
	 * Directories are created as needed from a specified position.
	 * 
	 * @param name The file path where the data will be written.
	 * @param data The content to be written to the file.
	 * @param user The user ID and group ID to set the file's ownership to; only sets if non-zero.
	 * @param pos The position in the path from which directories will be created;
	 *            directories before this position are not created.
	 */

	static void writeFile(string name, const string &data, uid_t user = 0, size_t pos = 0){
		if (! correctPath(name)) {
			Logger::log(LOG_ERR, "Trying to write an incorrect filename '%s'", name.c_str());
			throw HttpException(internalServerErrorCode, "I can't write file");
		}
		if (dirExists(name)) {
			Logger::log(LOG_ERR, "Trying to replace a dir with a file '%s'", name.c_str());
			throw HttpException(internalServerErrorCode, "I can't write file");
		}
		if (pathChanged(name, pos)) {
			Logger::log(LOG_ERR, "Trying go out of base directory with file '%s'", name.c_str());
			throw HttpException(internalServerErrorCode, "I can't write file");
		}
		string dir = getDirectory(name);
		if (dir.size()) {
			Logger::log(LOG_DEBUG, "path '%s' dir '%s'", name.c_str(), dir.c_str());
			if (!dirExists(dir))
				createDir(dir, user, pos);
		}
		int fileFd = openFileWithoutSymlinks(name, O_WRONLY | O_CREAT | O_TRUNC, 0600, pos);
		if (fileFd < 0) {
			Logger::log(LOG_ERR, "Can't open file '%s' for writing: %s (errno=%d)",
					name.c_str(), strerror(errno), errno);
			throw HttpException(internalServerErrorCode, "I can't write file");
		}
		const char *writeData = data.data();
		size_t remaining = data.size();
		while (remaining > 0) {
			ssize_t written = write(fileFd, writeData, remaining);
			if (written < 0 && errno == EINTR) continue;
			if (written <= 0) {
				close(fileFd);
				throw HttpException(internalServerErrorCode, "I can't write to file");
			}
			writeData += written;
			remaining -= written;
		}
		if (user) {
			if (fchown(fileFd, user, user))
				Logger::log(LOG_WARNING, "Can't change file owner %m");
		}
		bool isScript = name.size() >= 4 && name.substr(name.size() - 3) == ".sh";
		if (fchmod(fileFd, isScript ? 0700 : 0600))
			Logger::log(LOG_ERR, "Can't change file perm %m");
		close(fileFd);
	}

	/**
	 * Read a file
	 */
	static string readFile(string name, bool throwError = true, size_t pos = 0) {
		if (!correctPath(name) || pathChanged(name, pos)) {
			Logger::log(LOG_ERR,"Trying to read an incorrect filename '%s'", name.c_str());
			if(throwError)
				throw HttpException(internalServerErrorCode, "I can't read file");
			return "";
		}
		int fileFd = openFileWithoutSymlinks(name, O_RDONLY, 0, pos);
		if(fileFd < 0){
			if(throwError)
				throw HttpException(internalServerErrorCode
						,"I can't read file");
			return ""; 
		}
		string res;
		const int sbuffer=1024;
		char buffer[sbuffer];
		size_t read;
		while((read=::read(fileFd, buffer, sbuffer))>0){
			res+=string(buffer, read);
		}
		close(fileFd);
		return res;
	}

	/**
	 * Delete a file
	 * @param fileNamePath file path
	 * @param pos position in the path from which directories will be checked;
	 *            directories before this position are not checked.
	 */
	static void deleteFile(string fileNamePath, size_t pos = 0){
		if (!correctPath(fileNamePath)) return;
		string dirPath = getDirectory(fileNamePath);
		if (pathChanged(dirPath, pos)) {
			Logger::log(LOG_ERR,"Can't unlink \"%s\": is under symlink directory?", fileNamePath.c_str());
			return;
		}
		int parentFd = openFileWithoutSymlinks(dirPath.empty() ? "." : dirPath,
			O_RDONLY | O_DIRECTORY, 0, pos <= dirPath.size() ? pos : 0);
		if (parentFd < 0) return;
		string fileName = dirPath.empty() ? fileNamePath :
			fileNamePath.substr(dirPath.size() + 1);
		struct stat fileStat;
		if (fstatat(parentFd, fileName.c_str(), &fileStat, AT_SYMLINK_NOFOLLOW) == 0) {
			if (S_ISDIR(fileStat.st_mode)) {
				Logger::log(LOG_ERR,"Can't unlink \"%s\": is a directory", fileNamePath.c_str());
			} else if (S_ISREG(fileStat.st_mode)) {
				Logger::log(LOG_DEBUG,"Delete \"%s\"", fileNamePath.c_str());
				if (unlinkat(parentFd, fileName.c_str(), 0))
					Logger::log(LOG_ERR,"Can't unlink \"%s\": %m", fileNamePath.c_str());
			}
		}
		close(parentFd);
	}

	/**
	 * remove a directory and its content
	 * if force = true remove always
	 * else remove files owned by prisoner
	 * and complete directories owned by prisoner (all files and directories owns by prisoner or not)
	 */
	static int removeDir(string dir, uid_t owner, bool force) {
		if (dir == "/") {
			return 0;
		}
		string parent = getDirectory(dir);
		string name = parent.size() ? dir.substr(parent.size() + 1) : dir;
		if (parent.empty()) parent = ".";
		int parentFd = openFileWithoutSymlinks(parent, O_RDONLY | O_DIRECTORY);
		if (parentFd < 0) {
			return 0;
		}
		int directoryFd = openat(parentFd, name.c_str(), O_RDONLY | O_DIRECTORY | O_NOFOLLOW);
		if(directoryFd < 0){
			close(parentFd);
			Logger::log(LOG_ERR, "Can't open dir \"%s\": %m", dir.c_str());
			return 0;
		}
		struct stat directoryStat;
		if (fstat(directoryFd, &directoryStat) != 0) {
			close(directoryFd);
			close(parentFd);
			return 0;
		}
		bool removeAll = force || directoryStat.st_uid == owner || directoryStat.st_gid == owner;
		int nunlinks = removeDirContents(directoryFd, owner, removeAll);
		if (removeAll && unlinkat(parentFd, name.c_str(), AT_REMOVEDIR) == 0)
			nunlinks++;
		close(parentFd);
		return nunlinks;
	}
	/**
 	* Set/Unset socket operation int block/nonblock mode
 	*/
	static void fdblock(int fd, bool set=true){
		int flags;
		if( (flags = fcntl(fd, F_GETFL, 0)) < 0){
			Logger::log(LOG_ERR,"fcntl F_GETFL: %m");
		}
		if(set && (flags | O_NONBLOCK)==flags) flags ^=O_NONBLOCK;
		else flags |=O_NONBLOCK;
		if(fcntl(fd, F_SETFL, flags)<0){
			Logger::log(LOG_ERR,"fcntl F_SETFL: %m");
		}
	}

	/**
 	* Get the time of last modification of a file
 	*/
	static time_t timeOfFileModification(const string filePath){
		struct stat fileInfo;
		if(stat(filePath.c_str(), &fileInfo)>=0) {
			return fileInfo.st_mtime;
		} else {
			return 0;
		}
	}

	/**
	 * return server version
	 */
	static const char *version(){
		#ifdef VERSION
		return VERSION;
		#else
		return "Unknown";
		#endif
	}

	/**
	 * return server version
	 */
	static string URLdecode(const string& encoded){
		static string hexChars = "0123456789abcdefABCDEF";
		string decoded, hex;
		int len = encoded.length();
		decoded.reserve(len);
		hex.reserve(2);
		for (int i = 0; i < len; ++i) {
			char currentChar = encoded[i];
			if (currentChar == '%') {
				if (i + 2 >= len) {
					throw HttpException(badRequestCode,
						"URLdecode: incomplete percent-encoding sequence at the end of string");
				}
				hex = encoded.substr(i + 1, 2);

				if (hexChars.find(hex[0]) == string::npos || hexChars.find(hex[1]) == string::npos) {
					throw HttpException(badRequestCode,
						"URLdecode: non hex digits in percent-encoding sequence");
				}
				char value = static_cast<char>(std::strtol(hex.c_str(), NULL, 16));
				decoded.push_back(value);
				i += 2;
			} else if (currentChar == '+') {
				decoded.push_back(' '); // Convert + to space
			} else {
				decoded.push_back(currentChar); // Copy other characters
			}
		}
		return decoded;
	}

	static int get_utf8_nbytes_char(const string& text, size_t pos) {
		int num_bytes = -1;
		if (pos < text.size()) {
			unsigned char c = text[pos];
			if ((c >> 7) == 0) return 1;                 // ASCII
			if ((c >> 5) == 0b110) num_bytes = 2;        // 2-byte sequence
			else if ((c >> 4) == 0b1110) num_bytes = 3;  // 3-byte sequence
			else if ((c >> 3) == 0b11110) num_bytes = 4; // 4-byte sequence
			else if ((c >> 7)) return -1;                // Unexpected continuation or invalid byte
			if (pos + num_bytes > text.size()) return -1;
			for (int i = 1; i < num_bytes; i++) {
				c = text[pos + i];
				if ((c >> 6) != 0b10) return -(i + 1); // Ignore bad bytes
			}
		}
		else return 0;
		return num_bytes;
	}

	static string get_clean_utf8(const string& text) {
		string clean;
		clean.reserve(text.size());
		size_t pos = 0;
		while (pos < text.size()) {
			int num_bytes = get_utf8_nbytes_char(text, pos);
			if (num_bytes > 0) {
				clean.append(text.c_str() + pos, num_bytes);
				pos += num_bytes;
			} else if (num_bytes == 0) {
				return clean;
			} else {
				pos += (-num_bytes);
			}
		}
		return clean;
    }
};

#endif
