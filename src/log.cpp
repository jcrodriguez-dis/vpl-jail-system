/**
 * package:		Part of vpl-jail-system
 * copyright:	Copyright (C) 2023 Juan Carlos Rodríguez-del-Pino. All rights reserved.
 * license:		GNU/GPL, see LICENSE.txt or http://www.gnu.org/licenses/gpl-3.0.html
 **/

#include "log.h"

bool Logger::foreground = false;
int Logger::loglevel = 3;
const char * const Logger::levelName[8] = {
		"EMERG",
		"ALERT",
		"CRIT",
		"ERR",
		"WARNING",
		"NOTICE",
		"INFO",
		"DEBUG"
	};
