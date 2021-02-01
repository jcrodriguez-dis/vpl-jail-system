/**
 * package:		Part of vpl-jail-system
 * copyright:	Copyright (C) 2009 Juan Carlos Rodríguez-del-Pino. All rights reserved.
 * license:		GNU/GPL, see LICENSE.txt or http://www.gnu.org/licenses/gpl-3.0.html
 **/

#ifndef HTTPEXCEPTION_INC_H
#define HTTPEXCEPTION_INC_H
#include <string>
using namespace std;

enum CodeNumber {continueCode = 100,
	badRequestCode = 400,
	notFoundCode = 404,
	methodNotAllowedCode = 405,
	requestTimeoutCode = 408,
	requestEntityTooLargeCode = 413,
	internalServerErrorCode = 500,
	notImplementedCode = 501};
class HttpException {
	CodeNumber code;
	string message;
	string log;
public:
	HttpException(CodeNumber code, string message,string log="") {
		this->code=code;
		this->message=message;
		this->log=log;
	}
	CodeNumber getCode() {return code;}
	string getMessage() {return message;}
	string getLog() {return message+(log.size()?" ("+log+")":"");}
};

#endif
