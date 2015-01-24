/**
 * version:		$Id: util.h,v 1.23 2014/12/19 13:00:28 juanca Exp $
 * package:		Part of vpl-jail-system
 * copyright:	Copyright (C) 2009 Juan Carlos Rodríguez-del-Pino. All rights reserved.
 * license:		GNU/GPL, see LICENSE.txt or http://www.gnu.org/licenses/gpl-3.0.html
 **/

#ifndef VPL_UTIL_INC_H
#define VPL_UTIL_INC_H
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <syslog.h>
#include <stdio.h>
#include <regex.h>
#include <stdlib.h>
#include <dirent.h>
#include <signal.h>
#include <string.h>
#include <string>
#include <map>
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
			C642int[charSet[i]] = i<<2;
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
		if(ibyte>=data.size())return;
		//Sorry for to be so raw
		unsigned char *rawdata=(unsigned char *)data.c_str();
		rawdata[ibyte] |=(unsigned char)(dc>>ides);
		if(ides>2 && ibyte+1 < data.size()){
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
		ssize_t r=read(randomFile, ((char*)&ret), (sizeof (int)));
		return abs(ret+r);
	}
	/**
	 * clean the string removing spaces from end and start
	 * remove a 'text'=>text and "text"=>text
	 */
	static void trim(string &s){
		//FIXME is incorrect remove ' and "!!!!!
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
	 * Check if directory exists
	 */
	static bool dirExists(const string &fileName){
		struct stat info;
		return lstat(fileName.c_str(),&info)==0
				&& S_ISDIR(info.st_mode);
	}

	/**
	 * return an int/long int as string
	 */
	static string itos(const long int value){
		char buf[30];
		sprintf(buf,"%ld",value);
		return buf;
	}

	/**
	 * return an string as int
	 */
	static int atoi(const string &s){
		return ::atoi(s.c_str());
	}

	/**
	 * return an string as long int
	 */
	static int atol(const string &s){
		return ::atol(s.c_str());
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
		static bool init=false;
		static regex_t reg;
		if(!init){
			const char *sreg="[[:cntrl:]]|[!-,]|[:-@]|[{-~]|\\\\|\\[|\\]|[\\/\\^`]|^ | $|^\\-|\\.\\.";
			int res;
			if((res=regcomp(&reg,sreg , REG_EXTENDED))){
				char men[100];
				regerror(res,&reg,men,100);
				syslog(LOG_ERR,"Error: regcomp '%s' %s",sreg,men);
				throw "regcomp error";
			}
			init=true;
		}
		if(fn.size() <1) return false;
		if(fn.size() >JAIL_FILENAME_SIZE_LIMIT) return false;
		regmatch_t match[1];
		match[0].rm_so=0;
		match[0].rm_eo=0;
		int nomatch=regexec(&reg, fn.c_str(),1, match, 0);
		if(nomatch){
			syslog(LOG_DEBUG,"correctFile %d '%s' %d:%d"
					,nomatch,fn.c_str(),match[0].rm_so,match[0].rm_eo);
		}
		return nomatch==REG_NOMATCH;
	}

	static bool correctPath(const string &path){ //No trailing /
		if(path.size()==0) return false;
		if(path.size() > JAIL_PATH_SIZE_LIMIT) return false;
		size_t pos=0,found;
		string fn;
		if(path[0] == '/') pos=1; //skip absolute path
		while((found=path.find('/',pos)) != string::npos){
			fn=path.substr(pos,found-pos);
			if(!correctFileName(fn))
				return false;
			pos=found+1;
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

	static bool createDir(const string &path, uid_t user,size_t pos=0){ //absolute path
		string curDir;
		while((pos=path.find('/',pos)) != string::npos){
			curDir=path.substr(0,pos++);
			syslog(LOG_DEBUG,"dir to check or create '%s' user %d",curDir.c_str(),user);
			if(!dirExists(curDir)){
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
		if(!dirExists(path)){
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
	 * if force=true remove always
	 * else remove files owned by prisoner
	 * and complete directories owned by prisoner (all files and directories owns by prisoner or not)
	 */
	static int removeDir(string dir, uid_t owner,bool force){
		if(!dirExists(dir))
			return 0;
		const string parent(".."),me(".");
		int nunlink=0;
		struct stat filestat;
		dirent *ent;
		DIR *dirfd=opendir(dir.c_str());
		if(dirfd==NULL){
			syslog(LOG_ERR,"Can't open dir \"%s\": %m",dir.c_str());
			return 0;
		}
		while((ent=readdir(dirfd))!=NULL){
			const string name(ent->d_name);
			const string fullname=dir+"/"+name;
			lstat(fullname.c_str(),&filestat);
			bool owned=filestat.st_uid == owner || filestat.st_gid == owner;
			if(ent->d_type & DT_DIR){
				if(name != parent && name != me){
					nunlink+=removeDir(fullname,owner,force||owned);
					if((force || owned) && dirExists(fullname)){
						syslog(LOG_DEBUG,"Delete dir \"%s\"",fullname.c_str());
						if(rmdir(fullname.c_str())){
							syslog(LOG_ERR,"Can't rmdir \"%s\": %m",fullname.c_str());
						}
					}
				}
			}
			else{
				if(force || owned){
					syslog(LOG_DEBUG,"Delete \"%s\"",fullname.c_str());
					if(unlink(fullname.c_str())){
						syslog(LOG_ERR,"Can't unlink \"%s\": %m",fullname.c_str());
					}
					else nunlink++;
				}
			}
		}
		closedir(dirfd);
		if(force){
			syslog(LOG_DEBUG,"rmdir \"%s\"",dir.c_str());
			if(rmdir(dir.c_str())){
				syslog(LOG_ERR,"Can't rmdir \"%s\": %m",dir.c_str());
			}
		}
		return nunlink;
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
	 * return server version
	 */
	static const char *version(){
		return "2.1";
	}
};

#endif
