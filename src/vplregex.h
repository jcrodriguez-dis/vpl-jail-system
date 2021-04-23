/**
 * package:		Part of vpl-jail-system
 * copyright:	Copyright (C) 2021 Juan Carlos Rodríguez-del-Pino. All rights reserved.
 * license:		GNU/GPL, see LICENSE.txt or http://www.gnu.org/licenses/gpl-3.0.html
 **/

#ifndef VPLREGEX_INC_H
#define VPLREGEX_INC_H

/**
 * This file emulate C++ part of regex API using C regex functions
 */

#include <regex.h>
#include <vector>
#include <string>
using namespace std;

typedef vector<string> vplregmatch;


class vplregex {
	regex_t creg;
public:
	vplregex(const string &reg) {
		int res = regcomp(& creg, reg.c_str(), REG_EXTENDED);
		if (res) {
			static char buf[100];
			regerror(res, &creg, buf, 100);
			throw buf;
		}
	}
	~vplregex() {
		regfree(& creg);
	}
	bool search(const string &input, vplregmatch &found) {
		const int maxmatch = 10;
		int limit = (int) found.size();
		regmatch_t match[maxmatch];
		int nomatch = regexec(&creg, input.c_str(), maxmatch, match, 0);
		if (nomatch == REG_NOMATCH) return false;
		if (nomatch) {
			static char buf[100];
			regerror(nomatch, &creg, buf, 100);
			throw buf;
		}
		int nmatchs = limit > maxmatch? maxmatch : limit;
		for ( int i = 0; i < nmatchs; i++) {
			if (match[i].rm_so == -1) {
				found[i] = "";
			} else {
				found[i] = input.substr(match[i].rm_so,
	                                    match[i].rm_eo - match[i].rm_so);
			}
		}
		for ( int i = nmatchs; i < limit; i++) {
			found[i] = "";
		}
		return true;
	}
};

#endif
