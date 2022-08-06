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
string webSocket::getHandshakeAnswer(){
	string key = socket->getHeader("Sec-WebSocket-Key")
					+"258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
	string rec_pro=socket->getHeader("Sec-WebSocket-Protocol");
	string protocols;
	if(rec_pro.find("binary") != string::npos){
		//syslog(LOG_DEBUG,"Protocol binary");
		protocols = "Sec-WebSocket-Protocol: binary\r\n";
		base64 = false;
	}else if(rec_pro.find("base64") != string::npos){
		//syslog(LOG_DEBUG,"Protocol base64");
		base64 = true;
		protocols = "Sec-WebSocket-Protocol: base64\r\n";
	}else{
		base64 = false;
	}
	unsigned char sha1key[21];
	sha1key[20]=0;
	//syslog(LOG_DEBUG,"Websocket key: %s",key.c_str());
	SHA1((const unsigned char*)key.data(),key.size(),sha1key);
	//syslog(LOG_DEBUG,"Websocket SHA1 key: %s",sha1key);

	string responseKey=Base64::encode(string((char *)sha1key,20));
	//syslog(LOG_DEBUG,"responseKey : %s",responseKey.c_str());
	string ret="HTTP/1.1 101 Switching Protocols\r\n"
			"Connection: Upgrade\r\n"
			"Upgrade: websocket\r\n"
			VPL_SETIWASHERECOOKIE
			+ protocols +
			"Sec-WebSocket-Accept: " + responseKey + "\r\n\r\n";
	return ret;
}

long long webSocket::frameSize(const string &data
		,int &control_size,int &mask_size, long long &payload_size){
	control_size = 2;
	mask_size = 0;
	payload_size = 0;
	const unsigned char *rawdata = (const unsigned char *)data.data();
	if (data.size() < (unsigned long long) (control_size + mask_size + payload_size)) {
		return -1;
	}
	//Is masked frame (must be musked)
	mask_size = (rawdata[1] & 0x80) ? 4:0;
	payload_size = rawdata[1] & 0x7f;
	if( data.size() < (unsigned long long) (control_size + mask_size + payload_size)) {
		return -1;
	}
	if (payload_size == 126) {
		control_size += 2; //for len extension
		payload_size = (((unsigned int) rawdata[2]) << 8);
		payload_size += rawdata[3];
	}else if (payload_size == 127) {
		payload_size += ((unsigned long long) rawdata[5] << 32);
		payload_size += ((unsigned long long) rawdata[6] << 24);
		payload_size += ((unsigned long long) rawdata[7] << 16);
		payload_size += ((unsigned long long) rawdata[8] << 8);
		payload_size += rawdata[9];
	}
	return control_size + mask_size + payload_size;
}

bool webSocket::isFrameComplete(const string &data){
	int control_size, mask_size;
	long long payload_size;
	long long fSize = frameSize(data, control_size, mask_size, payload_size);
	if (fSize == -1) return false;
	return (int) data.size() >= fSize;
}

string webSocket::decodeFrame(string &data, FrameType &ft, bool &fin){
	int control_size, mask_size;
	long long payload_size;
	int fSize = frameSize(data, control_size, mask_size, payload_size);
	//syslog(LOG_DEBUG,"Decoding frame %d=%d+%d+%d",fSize,control_size,mask_size,payload_size);
	if(fSize == -1 || (int) data.size() < fSize){
		ft = ERROR_FRAME;
		return "Frame size too large";
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
	//syslog(LOG_DEBUG,"Frame type %d",(int)ft);
	string ret(payload_size, '\0');
	unsigned char *rawret = (unsigned char *)ret.c_str();
	const unsigned char *mask = (const unsigned char *)rawdata + control_size;
	const unsigned char *payload = (const unsigned char *)rawdata + (control_size + mask_size);
	for(int i = 0; i < payload_size; i++)
		rawret[i] = payload[i] ^ mask[i%4];
	data.erase(0, fSize);
	if (base64 && ft == TEXT_FRAME) {
		ft = BINARY_FRAME;
		string ret64 = Base64::decode( ret );
		//syslog(LOG_DEBUG,"Base64::decode %s",ret64.c_str());
		return ret64;
	}
	return ret;
}

string webSocket::encodeFrame(const string &rdata, FrameType ft){
	int control_size = 2;
	string data = rdata;
	if (base64 && ft == BINARY_FRAME) {
		data = Base64::encode(data);
		//syslog(LOG_DEBUG,"Base64::encode %s",data.c_str());
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
	static string previous_data;
	receiveBuffer += socket->receive();
	if (isFrameComplete(receiveBuffer)) {
		//syslog(LOG_INFO,"Websocket receive frame \"%s\"",receiveBuffer.c_str());
		bool fin;
		string data = decodeFrame(receiveBuffer, lFrameType, fin);
		//syslog(LOG_INFO,"Websocket receive type %d data \"%s\"",ft,data.c_str());
		switch (lFrameType) {
			case TEXT_FRAME:
			case BINARY_FRAME:
				if (fin) {
					if (previous_data.size() > 0 ) {
						data = previous_data + data;
						previous_data = "";
					}
					return data;
				} else {
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
	//syslog(LOG_INFO,"Websocket framing type %d data \"%s\"",ft,s.c_str());
	string frame = encodeFrame(s, ft);
	//syslog(LOG_INFO,"Frame send \"%s\"",frame.c_str());
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

