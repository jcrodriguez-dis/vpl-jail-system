/**
 * package:		Part of vpl-jail-system
 * copyright:	Copyright (C) 2021 Juan Carlos Rodríguez-del-Pino. All rights reserved.
 * license:		GNU/GPL, see LICENSE.txt or http://www.gnu.org/licenses/gpl-3.0.html
 **/
#include "basetest.h"
#include "../src/vplregex.h"
#include <cassert>
#include "../src/socket.h"
#include "../src/util.h"
#include "../src/jail.h"
#include "../src/redirector.h"

class VPLregexTest: public BaseTest {
	void testVPLRegex() {
		{
			vplregmatch found(4);
			string text = "GET http://algo.com/nada HTTP/1.2";
			Socket::regRequestLine.search(text, found);
			assert(found.size() == 4);
			assert(found[0] == text);
			assert(found[1] == "GET");
			assert(found[2] == "http://algo.com/nada");
			assert(found[3] == "HTTP/1.2");
		}
		{
			vplregmatch found(3);
			string text = "Cookie: name1=value1; name2=value2; name3=value3";
			Socket::regHeader.search(text, found);
			assert(found.size() == 3);
			assert(found[0] == text);
			assert(found[1] == "Cookie");
			assert(found[2] == "name1=value1; name2=value2; name3=value3");
		}
		{
			vplregmatch found(5);
			string text = "http://algo.com/nada.php?otracosa#1";
			assert(Socket::regURL.search(text, found));
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
			assert(Socket::regURL.search(text, found));
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
			Socket::regURL.search(text, found);
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
			Socket::regURL.search(text, found);
			assert(found.size() == 5);
			assert(found[0] == text);
			assert(found[1] == "");
			assert(found[2] == "");
			assert(found[3] == "/nada.php");
			assert(found[4] == "");
		}
		{
			vplregmatch found(5);
			string text = "/estoesunaclavelargalarga1234567890";
			Socket::regURL.search(text, found);
			assert(found.size() == 5);
			assert(found[0] == text);
			assert(found[1] == "");
			assert(found[2] == "");
			assert(found[3] == "/estoesunaclavelargalarga1234567890");
			assert(found[4] == "");
		}
		{
			vplregmatch found(5);
			string text = "/estoesunaclavelargalarga1234567890?";
			Socket::regURL.search(text, found);
			assert(found.size() == 5);
			assert(found[0] == text);
			assert(found[1] == "");
			assert(found[2] == "");
			assert(found[3] == "/estoesunaclavelargalarga1234567890");
			assert(found[4] == "");
		}
		{
			vplregmatch found(5);
			string text = "/estoesunaclavelargalarga1234567890?algo";
			Socket::regURL.search(text, found);
			assert(found.size() == 5);
			assert(found[0] == text);
			assert(found[1] == "");
			assert(found[2] == "");
			assert(found[3] == "/estoesunaclavelargalarga1234567890");
			assert(found[4] == "algo");
		}
		{
			vplregmatch found(5);
			string text = "/estoesunaclavelargalarga1234567890?algo#";
			Socket::regURL.search(text, found);
			assert(found.size() == 5);
			assert(found[0] == text);
			assert(found[1] == "");
			assert(found[2] == "");
			assert(found[3] == "/estoesunaclavelargalarga1234567890");
			assert(found[4] == "algo#");
		}
		{
			vplregmatch found(5);
			string text = "/estoesunaclavelargalarga1234567890?algo#algo";
			Socket::regURL.search(text, found);
			assert(found.size() == 5);
			assert(found[0] == text);
			assert(found[1] == "");
			assert(found[2] == "");
			assert(found[3] == "/estoesunaclavelargalarga1234567890");
			assert(found[4] == "algo#algo");
		}
		{
			vplregmatch found(5);
			string text = "/estoesunaclavelargalarga1234567890?algo#algo#";
			Socket::regURL.search(text, found);
			assert(found.size() == 5);
			assert(found[0] == text);
			assert(found[1] == "");
			assert(found[2] == "");
			assert(found[3] == "/estoesunaclavelargalarga1234567890");
			assert(found[4] == "algo#algo#");
		}
		{
			vplregmatch found(5);
			string text = "/";
			Socket::regURL.search(text, found);
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
			Socket::regURL.search(text, found);
			assert(found.size() == 5);
			assert(found[0] == text);
			assert(found[1] == "");
			assert(found[2] == "");
			assert(found[3] == "/");
			assert(found[4] == "algo");
		}
		{
			vplregmatch found(3);
			string text = "name1=value1; name2=value2; name3=value3";
			Socket::regCookie.search(text, found);
			assert(found.size() == 3);
			assert(found[0] == "name1=value1; ");
			assert(found[1] == "name1");
			assert(found[2] == "value1");
		}
		{
			vplregmatch found(3);
			string text = "name1=value1";
			Socket::regCookie.search(text, found);
			assert(found.size() == 3);
			assert(found[0] == text);
			assert(found[1] == "name1");
			assert(found[2] == "value1");
		}
		{
			vplregmatch found(3);
			string text = "name1=value1; ";
			Socket::regCookie.search(text, found);
			assert(found.size() == 3);
			assert(found[0] == text);
			assert(found[1] == "name1");
			assert(found[2] == "value1");
		}
		{
			vplregmatch found(3);
			string text = "127.11.52.24:34441";
			assert(RedirectorWebServer::regServerAddress.search(text, found));
			assert(found[0] == text);
			assert(found[1] == "127.11.52.24");
			assert(found[2] == "34441");
		}
		{
			vplregmatch found(3);
			string text = "/58495843242/4535034850953";
			assert(Jail::regWebSocketPath.search(text, found));
			assert(found[0] == text);
			assert(found[1] == "58495843242");
			assert(found[2] == "4535034850953");
			text = "/hola_jjjk  / sdlfjlsdfjlsaf  lf ";
			assert(Jail::regWebSocketPath.search(text, found));
			assert(found[0] == text);
			assert(found[1] == "hola_jjjk  ");
			assert(found[2] == " sdlfjlsdfjlsaf  lf ");
			text = "";
			assert(! Jail::regWebSocketPath.search(text, found));
			text = "/";
			assert(! Jail::regWebSocketPath.search(text, found));
		}
		{
			vplregmatch found(2);
			string text = "/.well-known/acme-challenge/algo";
			assert(Jail::regChallenge.search(text, found));
			assert(found[0] == text);
			assert(found[1] == "algo");
			text = "/.well-known/acme-challenge/algo kj klkjhk k hk";
			assert(Jail::regChallenge.search(text, found));
			assert(found[0] == text);
			assert(found[1] == "algo kj klkjhk k hk");
			text = "";
			assert(! Jail::regChallenge.search(text, found));
			text = "/hjg/gfg/gg";
			assert(! Jail::regChallenge.search(text, found));
			text = "/.well-known/acme-challenge/algokjk/lkjhkkhk";
			assert(! Jail::regChallenge.search(text, found));
			text = "/.well-known/acme-challenge/algokjk/../lkjhkkhk/";
			assert(! Jail::regChallenge.search(text, found));
		}
		{
			vplregmatch found(3);
			string text = "/58495843242/httpPassthrough";
			assert(Jail::regPassthroughTicket.search(text, found));
			assert(found[0] == text);
			assert(found[1] == "58495843242");
			text = "/hola_jjjk  /httpPassthrough";
			assert(Jail::regPassthroughTicket.search(text, found));
			assert(found[0] == text);
			assert(found[1] == "hola_jjjk  ");
			text = "//httpPassthrough";
			assert(! Jail::regPassthroughTicket.search(text, found));
			text = "/hola_jjjk  /httpPassthroug";
			assert(! Jail::regPassthroughTicket.search(text, found));
			text = "/58495843242/httppassthrough";
			assert(! Jail::regPassthroughTicket.search(text, found));
			text = "";
			assert(! Jail::regPassthroughTicket.search(text, found));
			text = "/";
			assert(! Jail::regPassthroughTicket.search(text, found));
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
