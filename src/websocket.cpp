/**
 * package:		Part of vpl-jail-system
 * copyright:	Copyright (C) 2013 Juan Carlos Rodríguez-del-Pino
 * license:		GNU/GPL, see LICENSE.txt or http://www.gnu.org/licenses/gpl-3.0.html
 **/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

//#include "sha1.h"
#include "util.h"
#include "websocket.h"

/*
 * RFC 6455
 * binary and base64 extension of noVNC supported
 */

/**
 * Validates that the given string is valid UTF-8 as required by RFC 6455 §8.1
 * for TEXT_FRAME payloads.
 */
static bool isValidUTF8(const string &s) {
	const unsigned char *bytes = (const unsigned char *)s.data();
	size_t len = s.size();
	size_t i = 0;
	while (i < len) {
		unsigned char b = bytes[i];
		int extra;
		unsigned long codepoint;
		if (b < 0x80) { i++; continue; }
		else if ((b & 0xE0) == 0xC0) { extra = 1; codepoint = b & 0x1F; }
		else if ((b & 0xF0) == 0xE0) { extra = 2; codepoint = b & 0x0F; }
		else if ((b & 0xF8) == 0xF0) { extra = 3; codepoint = b & 0x07; }
		else return false;
		if (i + (size_t)extra >= len) return false;
		for (int j = 1; j <= extra; j++) {
			unsigned char cb = bytes[i + j];
			if ((cb & 0xC0) != 0x80) return false;
			codepoint = (codepoint << 6) | (cb & 0x3F);
		}
		if (extra == 1 && codepoint < 0x80)    return false;
		if (extra == 2 && codepoint < 0x800)   return false;
		if (extra == 3 && codepoint < 0x10000) return false;
		if (codepoint > 0x10FFFF)              return false;
		if (codepoint >= 0xD800 && codepoint <= 0xDFFF) return false;
		i += 1 + extra;
	}
	return true;
}

/**
 * Replaces invalid UTF-8 byte sequences with the Unicode replacement character
 * U+FFFD (\xEF\xBF\xBD) so the result is always a valid UTF-8 string.
 * Used for TEXT_FRAME payloads that may contain raw process output.
 */
static string sanitizeUTF8(const string &s) {
	const unsigned char *bytes = (const unsigned char *)s.data();
	size_t len = s.size();
	string out;
	out.reserve(len);
	size_t i = 0;
	while (i < len) {
		unsigned char b = bytes[i];
		int extra;
		unsigned long codepoint;
		if (b < 0x80) { out += (char)b; i++; continue; }
		else if ((b & 0xE0) == 0xC0) { extra = 1; codepoint = b & 0x1F; }
		else if ((b & 0xF0) == 0xE0) { extra = 2; codepoint = b & 0x0F; }
		else if ((b & 0xF8) == 0xF0) { extra = 3; codepoint = b & 0x07; }
		else { out += '?'; i++; continue; } // invalid leading byte → '?' (size-preserving)
		bool valid = (i + (size_t)extra < len);
		if (valid) {
			for (int j = 1; j <= extra && valid; j++) {
				unsigned char cb = bytes[i + j];
				if ((cb & 0xC0) != 0x80) { valid = false; break; }
				codepoint = (codepoint << 6) | (cb & 0x3F);
			}
		}
		if (valid) {
			if ((extra == 1 && codepoint < 0x80) ||
			    (extra == 2 && codepoint < 0x800) ||
			    (extra == 3 && codepoint < 0x10000) ||
			    codepoint > 0x10FFFF ||
			    (codepoint >= 0xD800 && codepoint <= 0xDFFF)) {
				valid = false;
			}
		}
		if (valid) {
			for (int j = 0; j <= extra; j++) out += (char)bytes[i + j];
			i += 1 + extra;
		} else {
			out += '?'; // replace bad sequence with '?' (size-preserving: 1 byte per bad byte)
			i++;
		}
	}
	return out;
}

string webSocket::getHandshakeAnswer(){
	string key = socket->getHeader("Sec-WebSocket-Key")
					+"258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
	string rec_pro=socket->getHeader("Sec-WebSocket-Protocol");
	string protocols;
	if(rec_pro.find("binary") != string::npos){
		//Logger::log(LOG_DEBUG,"Protocol binary");
		protocols = "Sec-WebSocket-Protocol: binary\r\n";
		base64 = false;
	}else if(rec_pro.find("base64") != string::npos){
		//Logger::log(LOG_DEBUG,"Protocol base64");
		base64 = true;
		protocols = "Sec-WebSocket-Protocol: base64\r\n";
	}else{
		base64 = false;
	}
	unsigned char sha1key[21];
	sha1key[20]=0;
	//Logger::log(LOG_DEBUG,"Websocket key: %s",key.c_str());
	SHA1((const unsigned char*)key.data(),key.size(),sha1key);
	//Logger::log(LOG_DEBUG,"Websocket SHA1 key: %s",sha1key);

	string responseKey=Base64::encode(string((char *)sha1key,20));
	//Logger::log(LOG_DEBUG,"responseKey : %s",responseKey.c_str());
	string ret="HTTP/1.1 101 Switching Protocols\r\n"
			"Connection: Upgrade\r\n"
			"Upgrade: websocket\r\n"
			VPL_SETIWASHERECOOKIE
			+ protocols +
			"Sec-WebSocket-Accept: " + responseKey + "\r\n\r\n";
	return ret;
}

long long webSocket::frameSize(const string &data
		,int &control_size,int &mask_size, long long &payload_size) {
	long long int data_size = data.size();
	control_size = 2;
	mask_size = 0;
	payload_size = 0;
	const unsigned char *rawdata = (const unsigned char *)data.data();
	if (data_size < control_size) {
		return -1;
	}
	// The length is decoded separately from the sentinel values 126 and 127.
	mask_size = (rawdata[1] & 0x80) ? 4:0;
	unsigned int lengthCode = rawdata[1] & 0x7f;
	unsigned long long decodedSize;
	if (lengthCode < 126) {
		decodedSize = lengthCode;
	} else if (lengthCode == 126) {
		control_size = 4; //for len extension
		if (data_size < control_size) {
			return -1;
		}
		decodedSize = (((unsigned int) rawdata[2]) << 8) | rawdata[3];
		if (decodedSize < 126) return -2;
	} else {
		control_size = 10; //for len extension
		if (data_size < control_size) {
			return -1;
		}
		if (rawdata[2] & 0x80) return -2;
		decodedSize = ((unsigned long long) rawdata[2] << 56)
			| ((unsigned long long) rawdata[3] << 48)
			| ((unsigned long long) rawdata[4] << 40)
			| ((unsigned long long) rawdata[5] << 32)
			| ((unsigned long long) rawdata[6] << 24)
			| ((unsigned long long) rawdata[7] << 16)
			| ((unsigned long long) rawdata[8] << 8)
			| rawdata[9];
		if (decodedSize <= 0xffff) return -2;
	}
	if (decodedSize > JAIL_WEBSOCKET_FRAME_SIZE_LIMIT) return -2;
	if ((rawdata[0] & 0x0f) >= 8 && (rawdata[0] & 0x80) == 0) return -2;
	if ((rawdata[0] & 0x0f) >= 8 && decodedSize > 125) return -2;
	payload_size = (long long) decodedSize;
	if (data_size < (long long) control_size + mask_size + payload_size) {
		return -1;
	}
	return control_size + mask_size + payload_size;
}

bool webSocket::isFrameComplete(const string &data){
	int control_size, mask_size;
	long long payload_size;
	long long fSize = frameSize(data, control_size, mask_size, payload_size);
	if (fSize == -1) return false;
	if (fSize == -2) return true;
	return (long long) data.size() >= fSize;
}

string webSocket::decodeFrame(string &data, FrameType &ft, bool &fin){
	int control_size, mask_size;
	long long payload_size;
	long long fSize = frameSize(data, control_size, mask_size, payload_size);
	//Logger::log(LOG_DEBUG,"Decoding frame %d=%d+%d+%d",fSize,control_size,mask_size,payload_size);
	if(fSize == -1 || (long long) data.size() < fSize){
		ft = ERROR_FRAME;
		return "Frame size too large";
	}
	if (fSize == -2) {
		ft = ERROR_FRAME;
		return "Invalid frame length";
	}
	if (mask_size == 0) {
		ft = ERROR_FRAME;
		return "Frame must be masked";
	}
	const unsigned char *rawdata = (const unsigned char *)data.data();
	if (rawdata[0] & 0x70) {
		ft = ERROR_FRAME;
		return "Websocket extensions unsupported";
	}
	fin = (rawdata[0] & 0x80) > 0;
	if ((FrameType) (rawdata[0] & 0x0f) != CONTINUATION_FRAME) {
		ft = (FrameType) (rawdata[0] & 0x0f);
	}
	//Logger::log(LOG_DEBUG,"Frame type %d",(int)ft);
	string ret(payload_size, '\0');
	const unsigned char *mask = (const unsigned char *)rawdata + control_size;
	const unsigned char *payload = (const unsigned char *)rawdata + (control_size + mask_size);
	for(long long i = 0; i < payload_size; i++)
		ret[i] = payload[i] ^ mask[i%4];
	data.erase(0, fSize);
	if (base64 && ft == TEXT_FRAME) {
		ft = BINARY_FRAME;
		string ret64 = Base64::decode( ret );
		//Logger::log(LOG_DEBUG,"Base64::decode %s",ret64.c_str());
		return ret64;
	}
	return ret;
}

string webSocket::encodeFrame(const string &rdata, FrameType ft){
	int control_size = 2;
	string data = rdata;
	if (base64 && ft == BINARY_FRAME) {
		data = Base64::encode(data);
		//Logger::log(LOG_DEBUG,"Base64::encode %s",data.c_str());
		ft = TEXT_FRAME;
	}
	long long int payload_size = data.size();
	if (payload_size > 125)
		control_size += 2;
	if (payload_size > 0xFFFF)
		control_size += 6;
	string ret(control_size + payload_size, '\0');
	ret[0] = 0x80 | ft;
	if (payload_size <= 125) {
		ret[1] = payload_size;
	} else if (payload_size <= 0xffff) {
		ret[1] = 126;
		ret[2] = payload_size >> 8;
		ret[3] = payload_size & 0XFF;
	} else {
		ret[1] = 127;
		for(int i = 2 ; i < 5 ; i++)
			ret[i] = 0;
		ret[5] = (payload_size >> 32) & 0xff;
		ret[6] = (payload_size >> 24) & 0xff;
		ret[7] = (payload_size >> 16) & 0xff;
		ret[8] = (payload_size >> 8) & 0xff;
		ret[9] = payload_size & 0xff;
	}
	for(int i = 0; i < payload_size; i++)
		ret[i + control_size] = data[i];
	return ret;
}

webSocket::webSocket(Socket *s){
	socket=s;
	base64 = false;
	closeSent = false;
	socket->send(getHandshakeAnswer());
}

string webSocket::receive(){
	receiveBuffer += socket->receive();
	if (receiveBuffer.size() > JAIL_WEBSOCKET_FRAME_SIZE_LIMIT + 14 && !isFrameComplete(receiveBuffer)) {
		close("Error");
		socket->close();
		return "";
	}
	if (isFrameComplete(receiveBuffer)) {
		//Logger::log(LOG_INFO,"Websocket receive frame \"%s\"",receiveBuffer.c_str());
		bool fin;
		string data = decodeFrame(receiveBuffer, lFrameType, fin);
		//Logger::log(LOG_INFO,"Websocket receive type %d data \"%s\"",ft,data.c_str());
		switch (lFrameType) {
			case TEXT_FRAME:
			case BINARY_FRAME:
				if (fin) {
					if (previous_data.size() > 0 ) {
						data = previous_data + data;
						previous_data = "";
					}
					if (lFrameType == TEXT_FRAME && !isValidUTF8(data)) {
						// RFC 6455 §8.1: close with status 1007 (invalid frame payload data)
						string closePayload(2, '\0');
						closePayload[0] = static_cast<char>((1007 >> 8) & 0xFF);
						closePayload[1] = static_cast<char>(1007 & 0xFF);
						closePayload += "Invalid UTF-8 data";
						if (!closeSent) {
							socket->send(encodeFrame(closePayload, CONNECTION_CLOSE_FRAME));
							closeSent = true;
						}
						socket->close();
						return "";
					}
					return data;
				} else {
					if (previous_data.size() > JAIL_WEBSOCKET_FRAME_SIZE_LIMIT - data.size()) {
						close("Error");
						socket->close();
						return "";
					}
					previous_data += data;
					return "";
				}
				break;
			case CONNECTION_CLOSE_FRAME:
				{
					close("Bye");
					socket->close();
					break;
				}
			case PING_FRAME:
				{
					string pong=encodeFrame("Hello", PONG_FRAME);
					socket->send(pong);
					break;
				}
			case PONG_FRAME: //Do nothing
				break;
			default:
			case ERROR_FRAME:
				{
					close("Error");
					socket->close();
					break;
				}
		}
	}
	return "";
}

void webSocket::send(const string &s, FrameType ft){
	//Logger::log(LOG_INFO,"Websocket framing type %d data \"%s\"",ft,s.c_str());
	const string &payload = (ft == TEXT_FRAME && !isValidUTF8(s)) ? sanitizeUTF8(s) : s;
	string frame = encodeFrame(payload, ft);
	//Logger::log(LOG_INFO,"Frame send \"%s\"",frame.c_str());
	socket->send(frame);
}
void webSocket::close(string t){
	if(!closeSent){
		string bye = encodeFrame(t, CONNECTION_CLOSE_FRAME);
		socket->send(bye);
		closeSent = true;
	}
}

bool webSocket::wait(const int msec){
	if (receiveBuffer.size()>0) return false;
	return socket->wait(msec);
}
