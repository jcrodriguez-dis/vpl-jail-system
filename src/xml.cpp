/**
 * package:		Part of vpl-jail-system
 * copyright:	Copyright (C) 2009-20014 Juan Carlos Rodríguez-del-Pino. All rights reserved.
 * license:		GNU/GPL, see LICENSE.txt or http://www.gnu.org/licenses/gpl-3.0.html
 **/

#include "xml.h"

string TreeNodeXML::getString() const {
	if (tag == "string" || tag == "name")
		return XML::decodeXML(getRawContent());
	throw HttpException(badRequestCode
			, "Message data type error"
			, "Expected string and found " + tag );
}
