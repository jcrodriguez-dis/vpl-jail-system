/**
 * package:		Part of vpl-jail-system
 * copyright:	Copyright (C) 202" Juan Carlos Rodríguez-del-Pino. All rights reserved.
 * license:		GNU/GPL, see LICENSE.txt or http://www.gnu.org/licenses/gpl-3.0.html
 **/

#include "basetest.h"
#include <cassert>
#include "../src/xmlrpc.h"

class JSONRPCTest: public BaseTest {

public:
	string name() {
		return "JSONRPC class";
	}
	void launch() {
	}
};

JSONRPCTest jsonRpcTest;
