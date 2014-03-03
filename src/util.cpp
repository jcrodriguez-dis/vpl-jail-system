/**
 * version:		$Id: util.cpp,v 1.2 2014-02-11 18:02:07 juanca Exp $
 * package:		Part of vpl-jail-system
 * copyright:	Copyright (C) 2009 Juan Carlos Rodríguez-del-Pino. All rights reserved.
 * license:		GNU/GPL, see LICENSE.txt or http://www.gnu.org/licenses/gpl-3.0.html
 **/
#include "util.h"

int Base64::C642int[256];
static Base64 nothingBase64; 