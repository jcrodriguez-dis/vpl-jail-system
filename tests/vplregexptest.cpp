/**
 * package:		Part of vpl-jail-system
 * copyright:	Copyright (C) 2021 Juan Carlos Rodríguez-del-Pino. All rights reserved.
 * license:		GNU/GPL, see LICENSE.txt or http://www.gnu.org/licenses/gpl-3.0.html
 **/
#include "basetest.h"
#include "../src/vplregex.h"
#include <cassert>

class VPLregexTest: public BaseTest {
	void testVPLRegex() {
		vplregex regRequestLine("^([^ ]+) ([^ ]+) ([^ ]+)$");
		{
			vplregmatch found(4);
			string text = "GET http://algo.com/nada HTTP/1.2";
			regRequestLine.search(text, found);
			assert(found.size() == 4);
			assert(found[0] == text);
			assert(found[1] == "GET");
			assert(found[2] == "http://algo.com/nada");
			assert(found[3] == "HTTP/1.2");
		}
		vplregex regHeader("^[ \t]*([^ \t:]+):[ \t]*(.*)$");
		{
			vplregmatch found(3);
			string text = "Cookie: name1=value1; name2=value2; name3=value3";
			regHeader.search(text, found);
			assert(found.size() == 3);
			assert(found[0] == text);
			assert(found[1] == "Cookie");
			assert(found[2] == "name1=value1; name2=value2; name3=value3");
		}
		vplregex regURL("^([a-zA-Z]+)?(:\\/\\/[^\\/]+)?([^?]*)[?]?(.*)?$");
		{
			vplregmatch found(5);
			string text = "http://algo.com/nada.php?otracosa#1";
			assert(regURL.search(text, found));
			assert(found.size() == 5);
			assert(found[0] == text);
			assert(found[1] == "http");
			assert(found[2] == "://algo.com");
			assert(found[3] == "/nada.php");
			assert(found[4] == "otracosa#1");
		}
		{
			vplregmatch found(5);
			string text = "://nada.com/nada.php?otracosa#1";
			assert(regURL.search(text, found));
			assert(found.size() == 5);
			assert(found[0] == text);
			assert(found[1] == "");
			assert(found[2] == "://nada.com");
			assert(found[3] == "/nada.php");
			assert(found[4] == "otracosa#1");
		}
		{
			vplregmatch found(5);
			string text = "/nada.php?otracosa#1";
			regURL.search(text, found);
			assert(found.size() == 5);
			assert(found[0] == text);
			assert(found[1] == "");
			assert(found[2] == "");
			assert(found[3] == "/nada.php");
			assert(found[4] == "otracosa#1");
		}
		{
			vplregmatch found(5);
			string text = "/nada.php";
			regURL.search(text, found);
			assert(found.size() == 5);
			assert(found[0] == text);
			assert(found[1] == "");
			assert(found[2] == "");
			assert(found[3] == "/nada.php");
			assert(found[4] == "");
		}
		{
			vplregmatch found(5);
			string text = "/";
			regURL.search(text, found);
			assert(found.size() == 5);
			assert(found[0] == text);
			assert(found[1] == "");
			assert(found[2] == "");
			assert(found[3] == "/");
			assert(found[4] == "");
		}
		{
			vplregmatch found(5);
			string text = "/?algo";
			regURL.search(text, found);
			assert(found.size() == 5);
			assert(found[0] == text);
			assert(found[1] == "");
			assert(found[2] == "");
			assert(found[3] == "/");
			assert(found[4] == "algo");
		}
		vplregex regCookie("([^=]+)=([^;]+)(; )?");
		{
			vplregmatch found(3);
			string text = "name1=value1; name2=value2; name3=value3";
			regCookie.search(text, found);
			assert(found.size() == 3);
			assert(found[0] == "name1=value1; ");
			assert(found[1] == "name1");
			assert(found[2] == "value1");
		}
		{
			vplregmatch found(3);
			string text = "name1=value1";
			regCookie.search(text, found);
			assert(found.size() == 3);
			assert(found[0] == text);
			assert(found[1] == "name1");
			assert(found[2] == "value1");
		}
		{
			vplregex reg("^(127\\.[0-9]+\\.[0-9]+\\.[0-9]+):([0-9]+)$");
			vplregmatch found(3);
			string text = "127.11.52.24:34441";
			assert(reg.search(text, found));
			assert(found[0] == text);
			assert(found[1] == "127.11.52.24");
			assert(found[2] == "34441");
		}
		{
			vplregex webSocketPath("^\\/([^\\/]+)\\/(.+)$");
			vplregmatch found(3);
			string text = "/58495843242/4535034850953";
			assert(webSocketPath.search(text, found));
			assert(found[0] == text);
			assert(found[1] == "58495843242");
			assert(found[2] == "4535034850953");
			text = "/hola_jjjk  / sdlfjlsdfjlsaf  lf ";
			assert(webSocketPath.search(text, found));
			assert(found[0] == text);
			assert(found[1] == "hola_jjjk  ");
			assert(found[2] == " sdlfjlsdfjlsaf  lf ");
			text = "";
			assert(! webSocketPath.search(text, found));
			text = "/";
			assert(! webSocketPath.search(text, found));
			text = "/";
			assert(! webSocketPath.search(text, found));
			text = "/";
			assert(! webSocketPath.search(text, found));
		}
		{
			vplregex challenge("^\\/\\.well-known\\/acme-challenge\\/([^\\/]*)$");
			vplregmatch found(2);
			string text = "/.well-known/acme-challenge/algo";
			assert(challenge.search(text, found));
			assert(found[0] == text);
			assert(found[1] == "algo");
			text = "/.well-known/acme-challenge/algo kj klkjhk k hk";
			assert(challenge.search(text, found));
			assert(found[0] == text);
			assert(found[1] == "algo kj klkjhk k hk");
			text = "";
			assert(! challenge.search(text, found));
			text = "/hjg/gfg/gg";
			assert(! challenge.search(text, found));
			text = "/.well-known/acme-challenge/algokjk/lkjhkkhk";
			assert(! challenge.search(text, found));
			text = "/.well-known/acme-challenge/algokjk/../lkjhkkhk/";
			assert(! challenge.search(text, found));
		}
	}
public:
	string name() {
		return "VPLregex class";
	}
	void launch() {
		testVPLRegex();
	}
};
VPLregexTest vplregexTest;
