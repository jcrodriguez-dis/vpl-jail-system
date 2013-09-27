/**
 * version:		$Id: xml.h,v 1.7 2010-09-21 11:23:07 juanca Exp $
 * package:		Part of vpl-xmlrpc-jail
 * copyright:	Copyright (C) 2009 Juan Carlos Rodríguez-del-Pino. All rights reserved.
 * license:		GNU/GPL, see LICENSE.txt or http://www.gnu.org/licenses/gpl-2.0.html
 **/

#ifndef XML_INC_H
#define XML_INC_H
#include <stdlib.h>
#include <iconv.h>
#include <string>
#include <map>
#include <vector>
#include <syslog.h>

using namespace std;
/**
 * XML process XML data
 */
class XML{
	const string raw; //XML to process
public:
	/**
	 * Node that represent a tag
	 */
	class TreeNode{
		const string &raw;			//Reference to raw data
		const string tag;			//Tag name, no parm
		const size_t tag_offset;	//Tag offset in raw
		size_t tag_len;				//Length of tag information
		vector<TreeNode*> children;	//List of child tags
	public:
		TreeNode(const string &raw,string tag,size_t tag_offset):
		    raw(raw),tag(tag),tag_offset(tag_offset){
		    	tag_len=0;
		}
		~TreeNode(){
			for(size_t i=0; i<children.size(); i++){
				delete children[i];
			}
		}
		/**
		 * return tag name
		 */
		string getName() const {
			return tag;
		}
		/**
		 * return raw tag content
		 */
		string getContent() const {
			return raw.substr(tag_offset,tag_len);
		}
		/**
		 * return content as int
		 */
		int getInt() const {
			if(tag=="int")
				return atoi(getContent().c_str());
			syslog(LOG_ERR, "RPC/XML parse type error: expected int found %s",tag.c_str());
			throw "RPC/XML parse type error";
		}
		/**
		 * return content as string
		 */
		string getString() const {
			if(tag=="string")
				return XML::decodeXML(getContent());
			syslog(LOG_ERR, "RPC/XML parse type error: expected string found %s",tag.c_str());
			throw "RPC/XML parse type error";
		}
		size_t nchild() const{
			return children.size();
		}
		TreeNode* child(size_t i) const{
			if(i>=children.size())
				throw "RPC/XML parse child error";
			return children[i];
		}
		TreeNode* child(string stag) const{
			for(size_t i=0; i<children.size(); i++){
				if(children[i]->getName()==stag)
					return children[i];
			}
			throw "RPC/XML search child by name error";
		}

		/**
		 * Find the next tag from offset
		 * @param raw string complete XML
		 * @param offset search offset (update to the end of tag ">")
		 * @param tag string found
		 * @param btag where start the tag found
		 */
		static bool nextTag(const string &raw,size_t &offset, string &tag, size_t &btag){
			while(offset<raw.size()){
				if(raw[offset]=='<'){
					btag=offset;
					while(offset<raw.size()){
						if(raw[offset]=='>'){ //tag from btag i to etag
							tag=raw.substr(btag+1,offset-(btag+1));
							syslog(LOG_DEBUG,"tag: %s",tag.c_str());
							offset++;
							return true;
						}
						offset++;
					}
					throw "XML parse error: unexpected end of XML";
				}
				offset++;
			}
			return false;
		}
		/**
		 * parse content of a tag generating child
		 */
		size_t process(){
			size_t offset=tag_offset,ontag;
			string ntag;
			while(nextTag(raw,offset,ntag,ontag)){
				size_t l=ntag.size();
				if(ntag[0] == '/'){
					ntag.erase(0,1);
					if(tag==ntag){
						tag_len=ontag-tag_offset;
						return offset;
					}else{
						syslog(LOG_ERR,"Error unexpected end of tag: %s",ntag.c_str());
						throw "XML parse error: unexpected end of tag";
					}
				}else if(ntag[l-1]=='/'){
					ntag.erase(l-1,1);
					children.push_back(new TreeNode(raw,ntag,offset));
				}else{
					TreeNode *child=new TreeNode(raw,ntag,offset);
					children.push_back(child);
					offset = child->process();
				}
			}
			throw "XML parse error: end tag not found";
		}
	};
private:
	TreeNode *root; //root node of parse xml
public:
	//parse xml
	XML(const string &raw):raw(raw){
		size_t offset=0, aux;
		string tag;
		if(!TreeNode::nextTag(raw,offset,tag,aux) || !TreeNode::nextTag(raw,offset,tag,aux)){
			throw "XML parse error: begin tag not found";
		}
		root = new TreeNode(raw,tag,offset);
		root->process();
	}

	~XML(){
		delete root;
	}

	const TreeNode *getRoot(){
		return root;
	}

	static string encodeXML(const string &data){
		iconv_t cd=iconv_open("utf-8", "utf-8");
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
		size_t l=data.size();
		for(size_t i=0; i<l; i++){
			unsigned char c=output[i];
			if(c <= 0x1f){ //Control char
				if(c== '\n' || c=='\r' || c=='\t' ) //Acept TAB, LF and CR
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
			}
		}
		delete []output;
		return ret;
	}

	static string decodeXML(const string & data){
		string ret;
		for(size_t i=0; i<data.size(); i++){
			if(data[i]=='&'){
				i++;
				size_t begin=i;
				for(;i<data.size(); i++){
					if(data[i]==';'){
						string code=data.substr(begin,i-begin);
						if(code[0]=='#'){
							ret += (unsigned char)atoi(1+code.c_str());
						}else if(code=="amp"){
							ret += '&';
						}else if(code=="lt"){
							ret += '<';
						}else if(code=="gt"){
							ret += '>';
						}else if(code=="apos"){
							ret += '\'';
						}else if(code=="quot"){
							ret += '"';
						}
						else throw "XML string decode error";
						break;
					}
				}
				if(i>=data.size())
					throw "XML string decode error";
			}else{
				ret+=data[i];
			}
		}
		return ret;
	}

};

//mapstruct map of RPC-XML struct value
typedef map<string, const XML::TreeNode *> mapstruct;

#endif
