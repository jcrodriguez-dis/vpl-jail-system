/**
 * version:		$Id: configurationFile.h,v 1.2 2014-02-11 18:02:07 juanca Exp $
 * package:		Part of vpl-jail-system
 * copyright:	Copyright (C) 2009 Juan Carlos Rodríguez-del-Pino. All rights reserved.
 * license:		GNU/GPL, see LICENSE.txt or http://www.gnu.org/licenses/gpl-3.0.html
 **/

#ifndef CONFIGURATIONFILE_H_
#define CONFIGURATIONFILE_H_
#include <map>
#include <string>
using namespace std;
typedef map<string,string> ConfigData;
class ConfigurationFile{
	static void parseConfigLine(ConfigData &data, const string &line);
public:
	static ConfigData readConfiguration(string fileName, ConfigData defaultData);
	static void writeConfiguration(string fileName, ConfigData data);
};


#endif /* CONFIGURATIONFILE_H_ */
