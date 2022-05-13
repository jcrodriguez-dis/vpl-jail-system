/**
 * package:		Part of vpl-jail-system
 * copyright:	Copyright (C) 2009-2022 Juan Carlos Rodríguez-del-Pino. All rights reserved.
 * license:		GNU/GPL, see LICENSE.txt or http://www.gnu.org/licenses/gpl-3.0.html
 **/

#include "json.h"

string TreeNodeJSON::getString() const {
	return "";
	throw HttpException(badRequestCode
			, "Message data type error"
			, "Expected string and found " + tag );
}

string JSON::whiteSpaces = " \n\r\t";
