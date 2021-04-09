/**
 * package:		Part of vpl-jail-system
 * copyright:	Copyright (C) 2014 Juan Carlos Rodríguez-del-Pino. All rights reserved.
 * license:		GNU/GPL, see LICENSE.txt or http://www.gnu.org/licenses/gpl-3.0.html
 **/

#include "basetest.h"
#include <cassert>
#include "../src/util.h"

class UtilTest: public BaseTest {
	void testBase64Encode(){
		assert(Base64::encode("Hello")=="SGVsbG8="||(cerr <<Base64::encode("Hello")<<endl,0));
		assert(Base64::encode("abcde")=="YWJjZGU="||(cerr <<Base64::encode("abcde")<<endl,0));
		assert(Base64::encode("ñáÑ")=="w7HDocOR"||(cerr <<Base64::encode("ñáÑ")<<endl,0));
		assert(Base64::encode("ñáÑ=")=="w7HDocORPQ=="||(cerr <<Base64::encode("ñáÑ=")<<endl,0));
	}
	void testBase64Decode(){
		const int ibyte = 256;
		assert(Base64::decode("SGVsbG8=")=="Hello"||(cerr <<Base64::decode("SGVsbG8=")<<endl,0));
		assert(Base64::decode("YWJjZGU=")=="abcde"||(cerr <<Base64::decode("YWJjZGU=")<<endl,0));
		assert(Base64::decode("w7HDocOR")=="ñáÑ"||(cerr <<Base64::decode("w7HDocOR")<<endl,0));
		assert(Base64::decode("w7HDocORPQ==")=="ñáÑ="||(cerr <<Base64::decode("w7HDocORPQ==")<<endl,0));
		assert(Base64::decode(Base64::encode("RFB 003.008\n"))=="RFB 003.008\n");
		string test(ibyte, '\0');
		unsigned char *buf = (unsigned char *)test.data();
		for(int i = 0; i < ibyte;i++)
			buf[i]=i;
		string enc=Base64::encode(test);
		string dec=Base64::decode(enc);
		assert(dec.size() == ibyte);
		unsigned char *raw = (unsigned char *)dec.data();
		for(int i=0; i < ibyte;i++)
			assert(i == raw[i]);
	}
	void testTrim(){
		string val = ""; Util::trimAndRemoveQuotes(val); assert(val == "");
		val = "algo"; Util::trimAndRemoveQuotes(val); assert(val == "algo");
		val = "algo más"; Util::trimAndRemoveQuotes(val); assert(val == "algo más");
		val = "hola que     tal nada"; Util::trimAndRemoveQuotes(val); assert(val == "hola que     tal nada");
		val = "      "; Util::trimAndRemoveQuotes(val); assert(val == "");
		val = "  a  b "; Util::trimAndRemoveQuotes(val); assert(val == "a  b");
		val = "           a   ff  b    "; Util::trimAndRemoveQuotes(val); assert(val == "a   ff  b");
		val = "''"; Util::trimAndRemoveQuotes(val); assert(val == "");
		val = "\"\""; Util::trimAndRemoveQuotes(val); assert(val == "");
		val = "'  a  b '"; Util::trimAndRemoveQuotes(val); assert(val == "  a  b ");
		val = "\"  a  b \""; Util::trimAndRemoveQuotes(val); assert(val == "  a  b ");
		val = "   '  a  b '"; Util::trimAndRemoveQuotes(val); assert(val == "  a  b ");
		val = "       \"  a  b \"        "; Util::trimAndRemoveQuotes(val); assert(val == "  a  b ");
	}

	void testItos(){
		assert(Util::itos(0) == "0");
		assert(Util::itos(3) == "3");
		assert(Util::itos(-32780) == "-32780");
		assert(Util::itos(2038911111) == "2038911111");
		assert(Util::itos(-2038911111) == "-2038911111");
	}

	void testToUppercase(){
		assert(Util::toUppercase("") == "");
		assert(Util::toUppercase("a") == "A");
		assert(Util::toUppercase("a") == "A");
		assert(Util::toUppercase("á") == "á");
		assert(Util::toUppercase("ábCÑ()999  {ç") == "áBCÑ()999  {ç");
		assert(Util::toUppercase("jc\ndis") == "JC\nDIS");
		assert(Util::toUppercase("jc\\ndis") == "JC\\NDIS");
	}

	void testCorrectFileName(){
		assert(!Util::correctFileName("jc\ndis"));
		assert(!Util::correctFileName("jc\005dis"));
		assert(!Util::correctFileName("jc1di*s"));
		assert(!Util::correctFileName("jc1dis!"));
		assert(!Util::correctFileName("mal,c"));
		assert(!Util::correctFileName("mal:c"));
		assert(!Util::correctFileName("klsdjaf?"));
		assert(!Util::correctFileName("kl@sdjaf"));
		assert(!Util::correctFileName("kl'sdjaf"));
		assert(!Util::correctFileName("kl[sdjaf"));
		assert(!Util::correctFileName("klsdjaf^"));
		assert(!Util::correctFileName("kl\"sdjaf"));
		assert(!Util::correctFileName("klsdjaf\x60"));
		assert(!Util::correctFileName("(klsdjaf"));
		assert(!Util::correctFileName("klsd{jaf"));
		assert(!Util::correctFileName("klsdja~f"));
		assert(!Util::correctFileName("kls/djaf"));
		assert(!Util::correctFileName("\\klsdjaf.pp"));
		assert(!Util::correctFileName("nose..mal"));
		assert(!Util::correctFileName(""));
		assert(!Util::correctFileName(
				"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
				"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
				"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
				"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"));
		assert(Util::correctFileName("a"));
		assert(Util::correctFileName("kkkk"));
		assert(Util::correctFileName("a.raro"));
		assert(Util::correctFileName("normal.cvs"));
		assert(Util::correctFileName("normal.cvs.old"));
		assert(Util::correctFileName("normal-con-guiones.cvs"));
		assert(Util::correctFileName("normal_con_guiones.cvs"));
		assert(Util::correctFileName("b"));
		assert(Util::correctFileName("fichero con espacios. y varios . puntos"));
	}
	void testTimeOfFileModification(){
		string fileName = "timeOfFileModification.test_file";
		time_t now = time(NULL);
		Util::writeFile(fileName, fileName, geteuid());
		time_t modification = Util::timeOfFileModification(fileName);
		Util::deleteFile(fileName);
		assert(now <= modification &&  (modification - now) < 3 );
	}

	void testWriteReadRemoveFile(){
		Util::writeFile("/tmp/to_be_or_not_to_be", "mi texto único", getuid());
		assert( Util::fileExists("/tmp/to_be_or_not_to_be") );
		assert( Util::readFile("/tmp/to_be_or_not_to_be") == "mi texto único" );
		Util::deleteFile("/tmp/to_be_or_not_to_be");
		assert( ! Util::fileExists("/tmp/to_be_or_not_to_be") );
	}

	void testMemSizeToBytesl() {
		int long l1234 = 1234;
		int long kb1219 = 1219 * 1024;
		int long mb435 = 435 * 1024 * 1024;
		int long gb8 = 8l * 1024 * 1024 * 1024;
		assert(Util::memSizeToBytesl("nada") == 0);
		assert(Util::memSizeToBytesl("-10") == 0);
		assert(Util::memSizeToBytesl("0") == 0);
		assert(Util::memSizeToBytesl("0 Mb") == 0);
		assert(Util::memSizeToBytesl("1234") == l1234);
		assert(Util::memSizeToBytesl("1219 kbytes") == kb1219);
		assert(Util::memSizeToBytesl("1219K") == kb1219);
		assert(Util::memSizeToBytesl("435 Mbytes") == mb435);
		assert(Util::memSizeToBytesl("435m") == mb435);
		assert(Util::memSizeToBytesl("8gbytes") == gb8);
		assert(Util::memSizeToBytesl("8     G") == gb8);
	}

	void testMemSizeToBytesi() {
		int l1234 = 1234;
		int kb1219 = 1219 * 1024;
		int mb435 = 435 * 1024 * 1024;
		int gb1 = 1024 * 1024 * 1024;
		assert(Util::memSizeToBytesi("nada") == 0);
		assert(Util::memSizeToBytesi("-10") == 0);
		assert(Util::memSizeToBytesi("0") == 0);
		assert(Util::memSizeToBytesi("0 Mb") == 0);
		assert(Util::memSizeToBytesi("1234") == l1234);
		assert(Util::memSizeToBytesi("1219 kbytes") == kb1219);
		assert(Util::memSizeToBytesi("1219K") == kb1219);
		assert(Util::memSizeToBytesi("435 Mbytes") == mb435);
		assert(Util::memSizeToBytesi("435m") == mb435);
		assert(Util::memSizeToBytesi("1gbytes") == gb1);
		assert(Util::memSizeToBytesi("1     G") == gb1);
	}
public:
	string name() {
		return "Util class";
	}
	void launch() {
		testBase64Encode();
		testBase64Decode();
		testTrim();
		testItos();
		testToUppercase();
		testCorrectFileName();
		testWriteReadRemoveFile();
		testTimeOfFileModification();
		testMemSizeToBytesl();
		testMemSizeToBytesi();
	}
};
UtilTest utilTest;
