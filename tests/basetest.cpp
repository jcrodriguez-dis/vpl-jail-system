/**
 * package:		Part of vpl-jail-system
 * copyright:	Copyright (C) 2021 Juan Carlos Rodríguez-del-Pino. All rights reserved.
 * license:		GNU/GPL, see LICENSE.txt or http://www.gnu.org/licenses/gpl-3.0.html
 **/

#include "basetest.h"
vector<BaseTest*> BaseTest::tests;

int main() {
    return BaseTest::launchAll();
}
