/**
 * package:		Part of vpl-jail-system
 * copyright:	Copyright (C) 2009-20014 Juan Carlos Rodríguez-del-Pino. All rights reserved.
 * license:		GNU/GPL, see LICENSE.txt or http://www.gnu.org/licenses/gpl-3.0.html
 **/

#ifndef JSON_INC_H
#define JSON_INC_H
#include <stdlib.h>
#include <iconv.h>
#include <limits.h>
#include <string>
#include <sstream>
#include <map>
#include <vector>
#include <syslog.h>

#include "httpServer.h"
#include "requestmessage.h"

using namespace std;
class JSON;

class TreeNodeJSON: public TreeNode {
public:
	TreeNodeJSON(const string &raw, string tag, size_t tag_offset): TreeNode(raw, tag, tag_offset) {
	}
	/**
	 * return content as int
	 */
	int getInt() const {
		long long value = atoll(getRawContent().c_str());
		if ( value > INT_MAX ) {
			return INT_MAX;
		}
		return (int) value;
	}
	/**
	 * return content as long long
	 */
	long long getLong() const {
		return atoll(getRawContent().c_str());
	}
	string getString() const;
};

/**
 * XML data message
 */
class JSON: public RequestMessage {

	static bool isWhiteSpace(char c) {
		return c == ' ' || c == '\n' || c == '\r' || c == '\t';
	}
	/**
	 * Advance offset ignoring white spaces
	 * @param raw string complete JSON
	 * @param offset advance start point
	 * @return if no limit reached
	 */
	static void ignoreWhiteSpaces(const string &raw,size_t &offset){
		size_t limit = raw.size();
		while (offset < limit && isWhiteSpace(raw[offset])) {
			offset++;
		}
		if (offset >= limit) {
			throw HttpException(badRequestCode, "JSON parse error: unexpected en of JSON");
		};
	}

	void ignoreWhiteSpaces(size_t &offset){
		ignoreWhiteSpaces(raw, offset);
	}

	size_t processObjectNode(TreeNode *node) {
		size_t length = raw.size();
		size_t offset = node->getOffset() + 1;
		while( offset < length) {
			ignoreWhiteSpaces(offset);
			char c = raw[offset];
			if ( c == '}' ) {
				node->setLen(offset - node->getOffset());
				return offset + 1;
			} else if ( c == '"' ) {
				TreeNodeJSON name(raw, "name", offset);
				offset = processNode(&name);
				ignoreWhiteSpaces(offset);
				if ( raw[offset] != ':' ) {
					throw HttpException(badRequestCode, "JSON parse error: expected ':' not found");
				}
				offset++;
				ignoreWhiteSpaces(offset);
				TreeNode* newNodeValue = new TreeNodeJSON(raw, "value", offset);
				offset = processNode(newNodeValue);
				newNodeValue->setTag(name.getString());
				node->addChild(newNodeValue);
				ignoreWhiteSpaces(offset);
				if ( raw[offset] == ',' ) {
					offset++;
				} else if ( raw[offset] != '}' ) {
					throw HttpException(badRequestCode, "JSON parse error: Object bad format");
				}

			} else {
				throw HttpException(badRequestCode, "JSON parse error: unexpected char in object. Offset " + Util::itos(offset));
			}
		}
		throw HttpException(badRequestCode, "JSON parse error: end of object not found");
	}

	size_t processArrayNode(TreeNode *node) {
		size_t length = raw.size();
		size_t offset = node->getOffset() + 1;
		while (offset < length) {
			ignoreWhiteSpaces(offset);
			char c = raw[offset];
			if ( c == ']' ) {
				node->setLen(offset - node->getOffset());
				return offset + 1;
			} else {
				TreeNode* newNode = new TreeNodeJSON(raw, "node", offset);
				offset = processNode(newNode);
				node->addChild(newNode);
				ignoreWhiteSpaces(offset);
				if ( raw[offset] == ',' ) {
					offset++;
				} else if ( raw[offset] != ']' ) {
					throw HttpException(badRequestCode, "JSON parse error: Array bad format");
				}
			}
		}
		throw HttpException(badRequestCode, "JSON parse error: end of array not found");
	}

	size_t processStringNode(TreeNode *node) {
		size_t length = raw.size();
		node->setOffset(node->getOffset() + 1);
		for(size_t offset = node->getOffset(); offset < length; offset++) {
			char c = raw[offset];
			if ( c == '\\' ) {
				offset++;
				if (raw[offset] == 'u' && offset + 4 < length) {
					offset += 4;
				};
			} else if (c == '"') {
				node->setLen(offset - node->getOffset());
				return offset + 1;
			}
		}
		throw HttpException(badRequestCode, "JSON parse error: end of string not found");
	}

	size_t processNode(TreeNode *node) {
		size_t offset = node->getOffset();
		char c = raw[offset];
		if (c == '{') { // Objet
			node->setTag("object");
			return processObjectNode(node);
		} else if (c == '[') { //Array
			node->setTag("array");
			return processArrayNode(node);
		} else if (c == '"') { //String
			node->setTag("string");
			return processStringNode(node);
		} else if (c == '-' || std::isdigit(c)) {
			node->setTag("number");
			size_t limit = raw.size();
			while (offset < limit) {
				char c = raw[offset];
				if ( c == ',' || c == '}' || c == ']') {
					break;
				}
				offset ++;
			}
			node->setLen(offset - node->getOffset());
			return offset;
		} else if (raw.substr(offset, 4) == "true") {
			node->setTag("boolean");
			node->setLen(4);
			return offset + 4;			
		}else if (raw.substr(offset, 5) == "false") {
			node->setTag("boolean");
			node->setLen(5);
			return offset + 5;			
		} else if (raw.substr(offset, 4) == "null"){
			node->setTag("null");
			node->setLen(4);				
			return offset + 4;			
		} else {
				throw HttpException(badRequestCode
						,"JSON parse error: unexpected value", "Error found at offset " + Util::itos(offset));
		}
	}

	/**
	 * parse content of a tag generating child
	 */
	TreeNode *processRawData() {
		size_t offset=0;
		ignoreWhiteSpaces(offset);
		TreeNode* newroot = new TreeNodeJSON(raw, "root", offset);
		processNode(newroot);
		return newroot;
	}
	
public:
	//parse JSON
	JSON(const string &raw): RequestMessage(raw) {
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

	static string decodeHexaUnicodeToUTF8(const string &data, size_t &i) {
		if (i + 4 >= data.size()) {
			throw "JSON string coding \\u error";
		} else {
			size_t unicode;
			string result;
    		std::stringstream ss;
    		ss << std::hex << data.substr(i, 4);
    		ss >> unicode;
			if (unicode <= 0x007F) {
				result = "X";
				result[0] = (unsigned char) unicode;
			} else if (unicode <= 0x07FF) {
				result = "XX";
				result[0] = (unsigned char) (((unicode >> 6) & 0x1F) | 0xC0);
				result[1] = (unsigned char) ((unicode & 0x3F) | 0x80);
			} else {
				result = "XXX";
				result[0] = (unsigned char) (((unicode >> 12) & 0x0F) | 0xE0);
				result[1] = (unsigned char) (((unicode >>  6) & 0x3F) | 0x80);
				result[2] = (unsigned char) ((unicode & 0x3F) | 0x80);
			}
			i += 3;
			return result;
		}

	}
	static string decodeJSONString(const string &data, size_t i, size_t len) {
		string ret;
		size_t limit = i + len <= data.size() ? i + len : data.size();
		for(; i < limit; i++){
			char c = data[i];
			if ( c == '\\' ) {
				i++;
				char next = data[i];
				switch(next) {
					case '\\': ret += "\\";
					break;
					case '"': ret += "\"";
					break;
					case '/': ret += "/";
					break;
					case 'b': ret += "\b";
					break;
					case 'f': ret += "\f";
					break;
					case 'n': ret += "\n";
					break;
					case 'r': ret += "\r";
					break;
					case 't': ret += "\t";
					break;
					case 'u': 
						i++;
						ret += decodeHexaUnicodeToUTF8(data, i);
					break;
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

	static double decodeJSONNumber(const string &snumber) {
		double value;   
		std::stringstream ss(snumber);
		ss >> value;
		return value;
	}
};

#endif
