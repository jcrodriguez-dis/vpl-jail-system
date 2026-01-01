/**
 * package:		Part of vpl-jail-system
 * copyright:	Copyright (C) 2009 Juan Carlos Rodríguez-del-Pino. All rights reserved.
 * license:		GNU/GPL, see LICENSE.txt or http://www.gnu.org/licenses/gpl-3.0.html
 **/
#include "util.h"
#include <time.h>

int Base64::C642int[256];
static Base64 nothingBase64; 
const vplregex Util::regMemSize("^[ \t]*([0-9]+)[ \t]*([GgMmKk]?)");
const vplregex Util::correctFileNameReg("[[:cntrl:]]|[\"']|\\\\|[\\/\\^`]|^ | $|^\\.\\.$|^\\.$");

void Util::sleep(long microseconds) {
    struct timespec ts;
    ts.tv_sec = microseconds / 1000000;
    ts.tv_nsec = (microseconds % 1000000) * 1000;
    nanosleep(&ts, NULL);
}
