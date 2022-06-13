/**
 * package:		Part of vpl-jail-system
 * copyright:	Copyright (C) 2021 Juan Carlos Rodríguez-del-Pino. All rights reserved.
 * license:		GNU/GPL, see LICENSE.txt or http://www.gnu.org/licenses/gpl-3.0.html
 **/

#include "../src/httpException.h"
#include <vector>
#include <iostream>
#include <syslog.h>

using namespace std;

class BaseTest {
    static vector<BaseTest*> tests;
public:
    BaseTest() {
        tests.push_back(this);
    }
    virtual void launch()=0;
    virtual string name()=0;
    static int launchAll() {
        bool fail = false;
        cout << "Running " << tests.size() << " tests" << endl;
        for ( int i = 0; i <  tests.size(); i++) {
            BaseTest* test = tests[i];
            cout << (i+1) << ") Testing: " << test->name() << endl;
            try {
                test->launch();
            } catch(const exception & e) {
                cerr << "  Fail: " << e.what() << endl;
                if (! fail) {
                    openlog("vpl-jail-system-test",LOG_PID,LOG_DAEMON);
                    fail = true;
                }
				setlogmask(LOG_UPTO(LOG_INFO));
                try {
                    test->launch();
                } catch(const exception & e) {
                }
				setlogmask(LOG_UPTO(LOG_EMERG));                
            } catch (HttpException &e) {
                cerr << e.getMessage() << endl;
                if (! fail) {
                    openlog("vpl-jail-system-test",LOG_PID,LOG_DAEMON);
                    fail = true;
                }
				setlogmask(LOG_UPTO(LOG_INFO));
                try {
                    test->launch();
                } catch(const HttpException & e) {
                }
            }
		}
		cout << "Unit tests finished" << endl;
        return fail? 1 : 0;
    }
};
