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
#ifdef HAVE_WEAKLY_CANONICAL
#include <filesystem>
#endif
#include <sys/types.h>
#include <sys/stat.h>
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

#define VPL_EXECUTION "vpl_execution"
#define VPL_WEXECUTION "vpl_wexecution"
#define VPL_WEBEXECUTION "vpl_webexecution"
#define VPL_WEBCOOKIE "VPL_web"
#define VPL_SETWEBCOOKIE "Set-Cookie: " VPL_WEBCOOKIE "="
#define VPL_CLEANWEBCOOKIE VPL_SETWEBCOOKIE "n; Max-Age=-1\r\n"
#define VPL_IWASHERECOOKIE "VPL_Iwh"
#define VPL_SETIWASHERECOOKIE "Set-Cookie: " VPL_IWASHERECOOKIE "=y; Path=/; SameSite=none; Secure\r\n"
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
		int ibyte=pos/8;
		int ides=pos%8;
		int dc=decodeChar(c);
		int size = (int) data.size();
		if(ibyte>=size)return;
		//Sorry for to be so raw
		unsigned char *rawdata=(unsigned char *)data.c_str();
		rawdata[ibyte] |=(unsigned char)(dc>>ides);
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
		return kill(pid,0)==0;
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
	 * Retruns the directory path of a filepath
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
		string curDir;
		while((pos = path.find('/', pos)) != string::npos) {
			curDir = path.substr(0, pos);
			pos++;
			Logger::log(LOG_DEBUG, "Dir to check or create '%s' user %d", curDir.c_str(), user);
			if(! dirExists(curDir) ) {
				if(mkdir(curDir.c_str(), 0700)) {
					Logger::log(LOG_DEBUG, "Can't create dir '%s'", curDir.c_str());
					return false;
				}
				Logger::log(LOG_DEBUG, "Change owner to %d", user);
				if(lchown(curDir.c_str(), user, user)) {
					Logger::log(LOG_DEBUG, "Can't lchown dir '%s'", curDir.c_str());
					return false;
				}
			}
		}
		Logger::log(LOG_DEBUG, "Dir to check or create '%s'", path.c_str());
		if(! dirExists(path) ) {
			Logger::log(LOG_DEBUG, "Create '%s'",path.c_str());
			if(mkdir(path.c_str(), 0700)) {
				Logger::log(LOG_DEBUG, "Can't create dir '%s' %m", path.c_str());
				return false;
			}
			Logger::log(LOG_DEBUG, "Change owner to %d", user);
			if(lchown(path.c_str(),user,user)) {
				Logger::log(LOG_DEBUG, "Can't lchown dir '%s' %m", path.c_str());
				return false;
			}
		}
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
		// TODO when C++17 => use canonical or weakly_canonical from std::filesystem
		FILE *fd = fopen(name.c_str(), "wb");
		if (fd == NULL) {
			string dir = getDirectory(name);
			Logger::log(LOG_DEBUG, "path '%s' dir '%s'", name.c_str(), dir.c_str());
			if (dir.size())
				createDir(dir, user, pos);
			fd = fopen(name.c_str(), "wb");
			if (fd == NULL)
				throw HttpException(internalServerErrorCode, "I can't write file");
		}
		if (data.size() > 0 && fwrite(data.data(), data.size(), 1, fd) != 1) {
			fclose(fd);
			throw HttpException(internalServerErrorCode, "I can't write to file");
		}
		fclose(fd);
		if (user) {
			if (lchown(name.c_str(), user, user))
				Logger::log(LOG_WARNING, "Can't change file owner %m");
		}
		bool isScript = name.size() > 4 && name.substr(name.size() - 3) == ".sh";
		if (chmod(name.c_str(), isScript ? 0700 : 0600))
			Logger::log(LOG_ERR, "Can't change file perm %m");
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
		FILE *fd = fopen(name.c_str(),"rb");
		if(fd==NULL){
			if(throwError)
				throw HttpException(internalServerErrorCode
						,"I can't read file");
			return ""; 
		}
		string res;
		const int sbuffer=1024;
		char buffer[sbuffer];
		size_t read;
		while((read=fread(buffer,1,sbuffer,fd))>0){
			res+=string(buffer,read);
		}
		fclose(fd);
		return res;
	}

	/**
	 * Delete a file
	 * @param fileNamePath file path
	 * @param pos position in the path from which directories will be checked;
	 *            directories before this position are not checked.
	 */
	static void deleteFile(string fileNamePath, size_t pos = 0){
		if (dirExists(fileNamePath)) {
			Logger::log(LOG_ERR,"Can't unlink \"%s\": is a directory", fileNamePath.c_str());
			return;
		}
		string dirPath = getDirectory(fileNamePath);
		if (pathChanged(dirPath, pos)) {
			Logger::log(LOG_ERR,"Can't unlink \"%s\": is under symlink directory?", fileNamePath.c_str());
			return;
		}
		if (Util::fileExists(fileNamePath)) {
			Logger::log(LOG_DEBUG,"Delete \"%s\"", fileNamePath.c_str());
			if (unlink(fileNamePath.c_str())) {
				Logger::log(LOG_ERR,"Can't unlink \"%s\": %m", fileNamePath.c_str());
			}
		}
	}

	/**
	 * remove a directory and its content
	 * if force = true remove always
	 * else remove files owned by prisoner
	 * and complete directories owned by prisoner (all files and directories owns by prisoner or not)
	 */
	static int removeDir(string dir, uid_t owner, bool force) {
		if (! dirExists(dir)) {
			return 0;
		}
		const string parent(".."), me(".");
		int nunlinks = 0;
		struct stat filestat;
		lstat(dir.c_str(), &filestat);
		bool removeAll = force || (filestat.st_uid == owner || filestat.st_gid == owner);
		dirent *ent;
		DIR *dirfd = opendir(dir.c_str());
		if(dirfd==NULL){
			Logger::log(LOG_ERR, "Can't open dir \"%s\": %m", dir.c_str());
			return 0;
		}
		while((ent = readdir(dirfd)) != NULL) {
			const string name(ent->d_name);
			const string fullname = dir + "/" + name;
			lstat(fullname.c_str(), &filestat);
			if(S_ISDIR(filestat.st_mode)){
				if(name != parent && name != me){
					nunlinks += removeDir(fullname, owner, removeAll);
				}
			} else {
				bool remove = removeAll;
				if ( ! remove ) {
					lstat(fullname.c_str(), &filestat);
					remove = filestat.st_uid == owner || filestat.st_gid == owner;
				}
				if ( remove ) {
					Logger::log(LOG_DEBUG,"Delete \"%s\"", fullname.c_str());
					if( unlink(fullname.c_str()) ){
						Logger::log(LOG_ERR,"Can't unlink \"%s\": %m",fullname.c_str());
					} else {
						nunlinks++;
					}
				}
			}
		}
		closedir(dirfd);
		if (removeAll) {
			Logger::log(LOG_DEBUG,"rmdir \"%s\"",dir.c_str());
			if ( rmdir(dir.c_str()) ) {
				Logger::log(LOG_ERR,"Can't rmdir \"%s\": %m",dir.c_str());
			} else {
				nunlinks++;
			}
		}
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
		return "Unknow";
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
				char value = static_cast<char>(std::strtol(hex.c_str(), nullptr, 16));
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

	static int get_utf8_nbytes_char(const std::string& text, size_t pos) {
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

	static std::string get_clean_utf8(const std::string& text) {
		std::string clean;
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
