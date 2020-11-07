#!/bin/bash
# package:		Part of vpl-jail-system
# copyright:    Copyright (C) 2019 Juan Carlos Rodriguez-del-Pino
# license:      GNU/GPL, see LICENSE.txt or http://www.gnu.org/licenses/gpl.txt
# Description:  Script to install run tests for vpl-jail-system
cd ..
./configure
g++ -c *.cpp
rm main.o
cd tests
g++ *.cpp ../*.o -o test -lssl -lcrypto -lutil
rm ../*.o
if [ -x test ] ; then
   ./test
   rm test
fi
