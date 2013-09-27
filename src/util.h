/**
 * version:		$Id: util.h,v 1.5 2011-04-07 14:44:06 juanca Exp $
 * package:		Part of vpl-xmlrpc-jail
 * copyright:	Copyright (C) 2009 Juan Carlos Rodríguez-del-Pino. All rights reserved.
 * license:		GNU/GPL, see LICENSE.txt or http://www.gnu.org/licenses/gpl-2.0.html
 **/

#ifndef VPL_UTIL_INC_H
#define VPL_UTIL_INC_H
#include <stdio.h>
#include <string>
#include <map>
using namespace std;

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
	 * clean the string removing spaces from end and start
	 * remove a 'text'=>text and "text"=>text
	 */
	static void trim(string &s){
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
	 * Check if file exists
	 */
	static bool fileExists(const string &fileName){
		FILE *fd=fopen(fileName.c_str(),"rb");
		if(fd==NULL) return false;
		fclose(fd);
		return true;
	}

	/**
	 * return an int as string
	 */
	static string itos(const int value){
		char buf[30];
		sprintf(buf,"%d",value);
		return buf;
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
	 * return server version
	 */
	static const char *version(){
		return "1.1.1";
	}
};

#endif
