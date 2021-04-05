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
#include "vplregex.h"
#include "jail_limits.h"
#include "httpException.h"

#define VPL_EXECUTION "vpl_execution"
#define VPL_WEXECUTION "vpl_wexecution"
using namespace std;

enum ExitStatus {success=EXIT_SUCCESS,
				internalError=EXIT_FAILURE,
				neutral,
				httpError,
				websocketError};

struct ExecutionLimits{
	int maxtime;
	int maxfilesize;
	int maxmemory;
	int maxprocesses;
	void syslog(const char *s){
		::syslog(LOG_DEBUG,"%s: maxtime: %d seg, maxfilesize: %d Kb, maxmemory %d Kb, maxprocesses: %d",
				s,maxtime, maxfilesize/1024, maxmemory/1024, maxprocesses);
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
	 * return a random int take from /dev/random
	 */
	static int random(){
		int randomFile = open("/dev/urandom", O_RDONLY);
		int ret;
		ssize_t r = read(randomFile, ((void*) (&ret)), (sizeof(int)));
		return abs(ret+r);
	}
	/**
	 * clean the string removing spaces from end and start
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
	 * Check if process exists
	 */
	static bool processExists(pid_t pid){
		return kill(pid,0)==0;
	}

	/**
	 * Check if file exists
	 */
	static bool fileExists(const string &fileName){
		struct stat info;
		return lstat(fileName.c_str(),&info)==0
				&& S_ISREG(info.st_mode);
	}

	/**
	 * Check if directory exists, no symbolic links
	 */
	static bool dirExists(const string &fileName){
		struct stat info;
		int res = lstat(fileName.c_str(),&info);
		return res==0 && S_ISDIR(info.st_mode);
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
	 * return an int/long int as string
	 */
	static string itos(const long int value){
		const int maxIntChars = 31;
		char buf[maxIntChars];
		sprintf(buf,"%ld",value);
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
	 * return an string as long int
	 */
	static long int atol(const string &s){
		return ::atol(s.c_str());
	}

	/**
	 * return the value of Kb, Mb or Gb
	 */
	static long int memAbbreviation(const string &abbreviation){
		const long int kb = 1024;
		const long int mb = 1024 * kb;
		const long int gb = 1024 * mb;
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
		long int longValue = memSizeToBytesl(s);
		if ( longValue >  numeric_limits<int>::max()) {
			return numeric_limits<int>::max();
		}
		return longValue;
	}

	/**
	 * return a memory size in Gb, Mb or Kb to as bytes long int
	 */
	static long int memSizeToBytesl(const string &memSize){
		static vplregex regMemSize("^[ \t]*([0-9]+)[ \t]*([GgMmKk]?)");
		const int numberGroup = 1;
		const int abbrebiationGroup = 2;
		vplregmatch found;
		bool matchFound = regMemSize.search(memSize, found);
		if (matchFound) {
			return atol(found[numberGroup]) * memAbbreviation(found[abbrebiationGroup]);
		} else {
			return 0;
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
	static string getEnv(const string env){
		const char *res=getenv(env.c_str());
		if(res==NULL) return "";
		return res;
	}

	static bool correctFileName(const string &fn){
		static vplregex reg("[[:cntrl:]]|[!-,]|[:-@]|[{-~]|\\\\|\\[|\\]|[\\/\\^`]|^ | $|^\\-|\\.\\.");
		if(fn.size() <1) return false;
		if(fn.size() >JAIL_FILENAME_SIZE_LIMIT) return false;
		vplregmatch found;
		bool correct = ! reg.search(fn, found);
		if( ! correct){
			string incorrect = found[0];
			syslog(LOG_DEBUG,"incorrectFile '%s' found '%s'"
					,fn.c_str(), incorrect.c_str());
		}
		return correct;
	}

	static bool correctPath(const string &path){ //No trailing /
		if(path.size()==0) return false;
		if(path.size() > JAIL_PATH_SIZE_LIMIT) return false;
		size_t pos=0;
		size_t found;
		string fn;
		if(path[0] == '/') pos=1; //skip absolute path
		while((found=path.find('/',pos)) != string::npos){
			fn = path.substr(pos, found - pos);
			if(!correctFileName(fn))
				return false;
			pos = found + 1;
		}
		fn=path.substr(pos);
		return correctFileName(fn);
	}

	static string getDir(const string &filePath){
		size_t pos=0,found;
		if((pos=found=filePath.rfind('/')) != string::npos){
			return filePath.substr(0,pos);
		}
		return "";
	}

	static bool createDir(const string &path, uid_t user,size_t pos=1){ //absolute path
		string curDir;
		while((pos = path.find('/',pos)) != string::npos){
			curDir=path.substr(0,pos++);
			syslog(LOG_DEBUG,"dir to check or create '%s' user %d",curDir.c_str(),user);
			if(! dirExists(curDir) ){
				if(mkdir(curDir.c_str(),0700)){
					syslog(LOG_DEBUG,"Can't create dir '%s'",curDir.c_str());
					return false;
				}
				syslog(LOG_DEBUG,"Change owner to %d",user);
				if(lchown(curDir.c_str(),user,user)){
					syslog(LOG_DEBUG,"Can't lchown dir '%s'",curDir.c_str());
					return false;
				}

			}
		}
		syslog(LOG_DEBUG,"dir to check or create '%s'",path.c_str());
		if(! dirExists(path) ){
			syslog(LOG_DEBUG,"Create '%s'",path.c_str());
			if(mkdir(path.c_str(),0700)){
				syslog(LOG_DEBUG,"Can't create dir '%s' %m",path.c_str());
				return false;
			}
			syslog(LOG_DEBUG,"Change owner to %d",user);
			if(lchown(path.c_str(),user,user)){
				syslog(LOG_DEBUG,"Can't lchown dir '%s' %m",path.c_str());
				return false;
			}
		}
		return true;
	}
	//FIXME TODO with owner group
	/**
	 * Write a file
	 */
	static void writeFile(string name,const string &data,uid_t user=0,size_t pos=0){
		FILE *fd=fopen(name.c_str(),"wb");
		if(fd==NULL){
			string dir=getDir(name);
			syslog(LOG_DEBUG,"path '%s' dir '%s'",name.c_str(), dir.c_str());
			if(dir.size())
				createDir(dir,user,pos);
			fd=fopen(name.c_str(),"wb");
			if(fd==NULL)
				throw HttpException(internalServerErrorCode
						,"I can't write file");
		}
		if(data.size()>0 && fwrite(data.data(),data.size(),1,fd)!=1){
			fclose(fd);
			throw HttpException(internalServerErrorCode
					,"I can't write to file");
		}
		fclose(fd);
		if(lchown(name.c_str(),user,user))
			syslog(LOG_ERR,"Can't change file owner %m");
		bool isScript=name.size()>4 && name.rfind(".sh",name.size()-3)!=string::npos;
		if(chmod(name.c_str(),isScript?0700:0600))
			syslog(LOG_ERR,"Can't change file perm %m");
	}

	/**
	 * Read a file
	 */
	static string readFile(string name, bool throwError=true){
		FILE *fd=fopen(name.c_str(),"rb");
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
	 */
	static void deleteFile(string name){
		if(Util::fileExists(name)){
			syslog(LOG_DEBUG,"Delete \"%s\"",name.c_str());
			if(unlink(name.c_str())){
				syslog(LOG_ERR,"Can't unlink \"%s\": %m",name.c_str());
			}
		}
	}

	/**
	 * remove a directory and its content
	 * if force = true remove always
	 * else remove files owned by prisoner
	 * and complete directories owned by prisoner (all files and directories owns by prisoner or not)
	 */
	static int removeDir(string dir, uid_t owner,bool force){
		if(! dirExists(dir) ) {
			return 0;
		}
		const string parent(".."), me(".");
		int nunlinks = 0;
		struct stat filestat;
		lstat(dir.c_str(), &filestat);
		force = force || (filestat.st_uid == owner || filestat.st_gid == owner);

		dirent *ent;
		DIR *dirfd = opendir(dir.c_str());
		if(dirfd==NULL){
			syslog(LOG_ERR, "Can't open dir \"%s\": %m", dir.c_str());
			return 0;
		}
		while( ( ent = readdir(dirfd) ) != NULL){
			const string name(ent->d_name);
			const string fullname = dir + "/" + name;
			lstat(fullname.c_str(), &filestat);
			if(S_ISDIR(filestat.st_mode)){
				if(name != parent && name != me){
					nunlinks += removeDir(fullname, owner, force);
				}
			} else {
				bool remove = force;
				if ( ! remove ) {
					lstat(fullname.c_str(), &filestat);
					remove = filestat.st_uid == owner || filestat.st_gid == owner;
				}
				if ( remove ) {
					syslog(LOG_DEBUG,"Delete \"%s\"", fullname.c_str());
					if( unlink(fullname.c_str()) ){
						syslog(LOG_ERR,"Can't unlink \"%s\": %m",fullname.c_str());
					} else {
						nunlinks++;
					}
				}
			}
		}
		closedir(dirfd);
		if (force) {
			syslog(LOG_DEBUG,"rmdir \"%s\"",dir.c_str());
			if ( rmdir(dir.c_str()) ) {
				syslog(LOG_ERR,"Can't rmdir \"%s\": %m",dir.c_str());
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
			syslog(LOG_ERR,"fcntl F_GETFL: %m");
		}
		if(set && (flags | O_NONBLOCK)==flags) flags ^=O_NONBLOCK;
		else flags |=O_NONBLOCK;
		if(fcntl(fd, F_SETFL, flags)<0){
			syslog(LOG_ERR,"fcntl F_SETFL: %m");
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
		return VERSION;
	}
};

#endif
