/**
 * package:		Part of vpl-jail-system
 * copyright:	Copyright (C) 2009-2022 Juan Carlos Rodríguez-del-Pino. All rights reserved.
 * license:		GNU/GPL, see LICENSE.txt or http://www.gnu.org/licenses/gpl-3.0.html
 **/

#ifndef REQUESTMESSAGE_INC_H
#define REQUESTMESSAGE_INC_H
#include <stdlib.h>
#include <iconv.h>
#include <limits.h>
#include <string>
#include <map>
#include <vector>
#include <syslog.h>

#include "httpServer.h"

using namespace std;

/**
 * Node that represent a data
 */
class TreeNode {
protected:
	const string &raw;			//Reference to raw data
	string tag;					//Tag name, no parm
	size_t tag_offset;			//Tag offset in raw
	size_t tag_len;				//Length of tag information
	vector<TreeNode*> children;	//List of child tags
public:
	TreeNode(const string &raw, string tag, size_t tag_offset):
		raw(raw), tag(tag), tag_offset(tag_offset) {
		tag_len=0;
	}
	virtual ~TreeNode(){
		for(size_t i=0; i<children.size(); i++) {
			delete children[i];
		}
	}
	/**
	 * return tag name
	 */
	string getName() const {
		return tag;
	}

	size_t getOffset() const {
		return tag_offset;
	}

	size_t getLen() const {
		return tag_len;
	}

	void setLen(size_t len) {
		this->tag_len = len;
	}

	void setOffset(size_t offset) {
		this->tag_offset = offset;
	}

	void setTag(const string &tag) {
		this->tag = tag;
	}

	/**
	 * return raw tag content
	 */
	string getRawContent() const {
		return raw.substr(tag_offset, tag_len);
	}
	/**
	 * return content as int
	 */
	virtual int getInt() const {
		if (tag == "int") {
			long long value = atoll(getRawContent().c_str());
			if ( value > INT_MAX ) {
				return INT_MAX;
			}
			return (int) value;
		}
		if (tag == "double") {
			double value = atof(getRawContent().c_str());
			if(value > INT_MAX) return INT_MAX;
			return (int) value;
		}
		throw HttpException(badRequestCode
				, "Message data type error"
				, "Expected int found " + tag);
	}
	/**
	 * return content as long long
	 */
	virtual long long getLong() const {
		if (tag == "int") {
			long long value = atoll(getRawContent().c_str());
			return value;
		}
		if (tag == "double") {
			double value = atof(getRawContent().c_str());
			if(value > INT_MAX) return INT_MAX;
			return (long long) value;
		}
		throw HttpException(badRequestCode
				, "Message data type error"
				, "Expected int found " + tag);
	}
	/**
	 * return content as string
	 */
	virtual string getString() const = 0;

	void addChild(TreeNode *node) {
		children.push_back(node);
	}
	size_t nchild() const {
		return children.size();
	}
	TreeNode* child(size_t i) const {
		if (i >= children.size())
			throw HttpException(badRequestCode
				, "Message data structure error"
				, "The index of child requested does not exist");
		return children[i];
	}
	TreeNode* child(string stag) const {
		for(size_t i=0; i<children.size(); i++) {
			if(children[i]->getName() == stag)
				return children[i];
		}
		throw HttpException(badRequestCode
				,"The child or attribute name requested does not exist " + stag);
	}
};


/**
 * Request Message
 */
class RequestMessage {
protected:
	const string raw;
public:
protected:
	virtual size_t processNode(TreeNode *) = 0;
	virtual TreeNode *processRawData() = 0;
	TreeNode *root; //root node of parsed message
public:
	
	RequestMessage(const string &rawdata):raw(rawdata) {
		root = NULL;
	}

	virtual ~RequestMessage() {
		delete root;
	}

	TreeNode *getRoot() {
		return root;
	}
};

//mapstruct map of RequestMessage struct value
typedef map<string, const TreeNode *> mapstruct;

#endif
