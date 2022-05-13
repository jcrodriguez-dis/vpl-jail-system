/**
 * package:		Part of vpl-jail-system
 * copyright:	Copyright (C) 2009-20014 Juan Carlos Rodríguez-del-Pino. All rights reserved.
 * license:		GNU/GPL, see LICENSE.txt or http://www.gnu.org/licenses/gpl-3.0.html
 **/

#ifndef XML_INC_H
#define XML_INC_H
#include <stdlib.h>
#include <iconv.h>
#include <limits.h>
#include <string>
#include <sstream>
#include <map>
#include <vector>
#include <syslog.h>

#include "httpServer.h"
#include "datamessage.h"

using namespace std;
class JSON;

class TreeNodeJSON: public TreeNode {
public:
	TreeNodeJSON(const string &raw, string tag, size_t tag_offset): TreeNode(raw, tag, tag_offset) {
	}

	string getString() const;
};

/**
 * XML data message
 */
class JSON: public DataMessage {
	static string whiteSpaces;
	/**
	 * Find the next tag from offset
	 * @param raw string complete JSON
	 * @param offset search offset
	 * @param tag string found
	 * @param btag where start the tag found
	 */
	static bool nextTag(const string &raw,size_t &offset, string &tag, size_t &btag){
		while (offset < raw.size()) {
			if (raw[offset] == '<') {
				btag=offset;
				while (offset < raw.size()) {
					if (raw[offset] == '>'){ //tag from btag i to etag
						tag = raw.substr(btag + 1, offset - (btag + 1));
						//syslog(LOG_DEBUG,"tag: %s",tag.c_str());
						offset++;
						return true;
					}
					offset++;
				}
				throw HttpException(badRequestCode
						,"XML parse error: unexpected end of XML");
			}
			offset++;
		}
		return false;
	}
	size_t processNode(TreeNode *node) {
		size_t offset = node->getOffset(), ontag;
		string ntag;
		while (nextTag(raw, offset, ntag, ontag)) {
			size_t l = ntag.size();
			if (ntag[0] == '/') {
				ntag.erase(0,1);
				if (node->getName() == ntag) {
					node->setLen(ontag - node->getOffset());
					return offset;
				} else {
					throw HttpException(badRequestCode
							,"XML parse error: unexpected end of tag",ntag);
				}
			} else if (ntag[l - 1] == '/') {
				ntag.erase(l - 1,1);
				node->addChild(new TreeNodeJSON(raw,ntag,offset));
			} else {
				TreeNode *child = new TreeNodeJSON(raw, ntag, offset);
				node->addChild(child);
				offset = processNode(child);
			}
		}
		throw HttpException(badRequestCode
				,"XML parse error: end tag not found");
	}
	/**
	 * parse content of a tag generating child
	 */
	TreeNode *processRawData() {
		size_t offset=0, aux;
		if(raw.find("</methodCall>", raw.size() - 13) != string::npos){
			syslog(LOG_INFO,"XML: data pass end tag of methodcall");
		}
		string tag;
		if(!nextTag(raw, offset, tag, aux) || !nextTag(raw,offset,tag,aux)){
			throw HttpException(badRequestCode
					,"XML parse error: start tag not found");
		}
		TreeNode* newroot = new TreeNodeJSON(raw, tag, offset);
		processNode(newroot);
		return newroot;
	}
	
public:
	//parse JSON
	JSON(const string &raw): DataMessage(raw) {
		root = processRawData();
	}

	static string encodeJSONString(const string &data) {
		string ret;
		for(size_t i=0; i<data.size(); i++){
			char c = data[i];
			switch(c) {
				case '\\':ret += "\\\\";
				break;
				case '"':ret += "\\\"";
				break;
				case '/':ret += "\\/";
				break;
				case '\b':ret += "\\b";
				break;
				case '\f':ret += "\\f";
				break;
				case '\n':ret += "\\n";
				break;
				case '\r':ret += "\\r";
				break;
				case '\t':ret += "\\t";
				break;
				default: ret += c;
				break;
			}
		}
		return ret;
	}

	static string decodeHexadigits(const string &data, size_t &i) {
		if (i + 4 >= data.size()) {
			throw "JSON string coding \\u error";
		} else {
			size_t value;   
    		std::stringstream ss;
    		ss << std::hex << data.substr(i + 1, 4);
    		ss >> value;
			string result = "XX";
			result[0] = (unsigned char) (value / 256);
			result[1] = (unsigned char) (value % 256);
			i += 4;
			return result;
		}

	}
	static string decodeJSONString(const string &data, size_t &i) {
		string ret;
		size_t length = data.size();
		for(; i < length; i++){
			char c = data[i];
			if ( c == '\\' ) {
				i++;
				char next = data[i];
				switch(next) {
					case '\\':ret += "\\";
					break;
					case '"':ret += "\"";
					break;
					case '/':ret += "/";
					break;
					case 'b':ret += "\b";
					break;
					case 'f':ret += "\f";
					break;
					case 'n':ret += "\n";
					break;
					case 'r':ret += "\r";
					break;
					case 't':ret += "\t";
					break;
					case 'u':
						ret += decodeHexadigits(data, i);
					default: 
						throw "JSON coding error: bad escape secuence";
					break;
				}

			} else {
				if (c == '"') {
					return ret;
				}
				ret += c;
			}
		}
		return ret;
	}

	static double decodeJSONNumber(const string &data, size_t &i) {
		string snumber;
		size_t length = data.size();
		string c = "X";
		for(; i < length; i++) {
			c[0] = data[i];
			if ( whiteSpaces.find(c) != string::npos ) {
				break;
			}
			snumber += c;
		}
		if (snumber.size() == 0) {
			throw "JSON string coding \\u error";
		}
		double value;   
		std::stringstream ss(snumber);
		ss >> value;
		return value;
	}
};

#endif
