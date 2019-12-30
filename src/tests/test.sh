#!/bin/bash
# package:		Part of vpl-jail-system
# copyright:    Copyright (C) 2019 Juan Carlos Rodriguez-del-Pino
# license:      GNU/GPL, see LICENSE.txt or http://www.gnu.org/licenses/gpl.txt
# Description:  Script to install run tests for vpl-jail-system

g++ test.cpp ../util.cpp ../configuration.cpp ../configurationFile.cpp -o test
./test

