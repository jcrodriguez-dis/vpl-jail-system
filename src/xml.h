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
#include <map>
#include <vector>
#include <syslog.h>

#include "httpServer.h"
#include "requestmessage.h"

using namespace std;
class XML;

class TreeNodeXML: public TreeNode {
public:
	TreeNodeXML(const string &raw, string tag, size_t tag_offset): TreeNode(raw, tag, tag_offset) {
	}

	string getString() const;
};

/**
 * XML data message
 */
class XML: public RequestMessage {
	/**
	 * Find the next tag from offset
	 * @param raw string complete XML
	 * @param offset search offset (update to the end of tag ">")
	 * @param tag string found
	 * @param btag where start the tag found
	 */
	static bool nextTag(const string &raw, size_t &offset, string &tag, size_t &btag){
		while (offset < raw.size()) {
			if (raw[offset] == '<') {
				btag = offset;
				while (offset < raw.size()) {
					if (raw[offset] == '>'){ //tag from btag i to etag
						tag = raw.substr(btag + 1, offset - (btag + 1));
						//syslog(LOG_DEBUG,"tag: %s",tag.c_str());
						offset++;
						return true;
					}
					offset++;
				}
				throw HttpException(badRequestCode, "XML parse error: unexpected end of XML");
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
							,"XML parse error: unexpected end of tag", ntag);
				}
			} else if (ntag[l - 1] == '/') {
				ntag.erase(l - 1, 1);
				node->addChild(new TreeNodeXML(raw, ntag, offset));
			} else {
				TreeNode *child = new TreeNodeXML(raw, ntag, offset);
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
		if(raw.find("</methodCall>", raw.size() - 15) == string::npos){
			syslog(LOG_INFO,"XML: data pass end tag of methodcall");
		}
		string tag;
		if(!nextTag(raw, offset, tag, aux) || !nextTag(raw,offset,tag,aux)){
			throw HttpException(badRequestCode
					,"XML parse error: start tag not found");
		}
		TreeNode* newroot = new TreeNodeXML(raw, tag, offset);
		processNode(newroot);
		return newroot;
	}
	
public:
	//parse xml
	XML(const string &raw): RequestMessage(raw) {
		root = processRawData();
	}

	static string encodeXML(const string &data){
		iconv_t cd=iconv_open("UTF-8//TRANSLIT", "UTF-8");
		char *chars=(char *)data.c_str();
		size_t inbytesleft=data.size();
		size_t outbytesleft=data.size();
		char *output= new char[inbytesleft+1];
		char *out=output;
		//Filter not UTF-8 chars
		while(inbytesleft>0){
			size_t res=iconv(cd,&chars,&inbytesleft,&out,&outbytesleft);
			if(res == (size_t)(-1)){
				*out=126;
				out++;
				outbytesleft--;
				chars++;
				inbytesleft--;
			}
		}
		iconv_close(cd);
		//Convert entities
		string ret;
		size_t l=out-output;
		for(size_t i=0; i<l; i++){
			unsigned char c=output[i];
			if(c <= 0x1f){ //Control char
				if(c== '\n' || c=='\r' || c=='\t' ) //Accept TAB, LF and CR
					ret += c;
				else
					ret += 126; //Change by tilde
			}
			else
				switch(c){
				case '&':ret += "&amp;";
				break;
				case '<':ret += "&lt;";
				break;
				case '>':ret += "&gt;";
				break;
				case '\'':ret += "&apos;";
				break;
				case '"':ret += "&quot;";
				break;
				case 127:ret += 126;
				break;
				default: ret += c;
				break;
				}
		}
		delete []output;
		return ret;
	}

	static string decodeXML(const string & data) {
		string ret;
		for(size_t i = 0; i < data.size(); i++) {
			if(data[i] == '&') {
				i++;
				size_t begin = i;
				for(; i < data.size(); i++) {
					if(data[i] == ';') { 
						string code = data.substr(begin, i - begin);
						if(code[0] == '#') {
							ret += (unsigned char) atoi(1 + code.c_str());
						} else if(code == "amp") {
							ret += '&';
						} else if(code == "lt") {
							ret += '<';
						} else if(code == "gt") {
							ret += '>';
						} else if(code == "apos") {
							ret += '\'';
						} else if(code == "quot") {
							ret += '"';
						} else throw HttpException(badRequestCode, "XML string decode error");
						break;
					}
				}
				if(i >= data.size()) {
					throw HttpException(badRequestCode, "XML string decode error");
				}
			} else {
				ret += data[i];
			}
		}
		return ret;
	}
};

#endif
