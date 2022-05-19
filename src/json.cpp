/**
 * package:		Part of vpl-jail-system
 * copyright:	Copyright (C) 2009-2022 Juan Carlos Rodríguez-del-Pino. All rights reserved.
 * license:		GNU/GPL, see LICENSE.txt or http://www.gnu.org/licenses/gpl-3.0.html
 **/

#include "json.h"

string TreeNodeJSON::getString() const {
	if (this->nchild() > 0) {
		return "Object/Array";
	}
	return JSON::decodeJSONString(this->raw, this->getOffset(), this->getLen());
}
