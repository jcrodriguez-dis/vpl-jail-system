/**
 * package:		Part of vpl-jail-system
 * copyright:	Copyright (C) 2013 Juan Carlos Rodríguez-del-Pino
 * license:		GNU/GPL, see LICENSE.txt or http://www.gnu.org/licenses/gpl-3.0.html
 **/
#ifndef WEBSOCKET_H_
#define WEBSOCKET_H_
#include "socket.h"

enum FrameType {
	CONTINUATION_FRAME = 0x00,
	TEXT_FRAME = 0x01,
	BINARY_FRAME = 0x02,
	CONNECTION_CLOSE_FRAME = 0x08,
	PING_FRAME = 0x09,
	PONG_FRAME = 0x0A,
	ERROR_FRAME =0xFF
};
class webSocket{
	bool closeSent;
	bool base64;
	Socket *socket;
	string receiveBuffer;
	string getHandshakeAnswer();
	int frameSize(const string &data
			,int &control_size,int &mask_size, int &payload_size);
	bool isFrameComplete(const string &data);
	string decodeFrame(string &data, FrameType &);
	string encodeFrame(const string &rdata, FrameType);
public:
	webSocket(Socket *s);
	int getSocket(){return socket->getSocket();}
	void close(string t="");
	bool isReadBuffered() { return socket->isReadBuffered() || receiveBuffer.size()>0;}
	bool isWriteBuffered() { return socket->isWriteBuffered();}
	bool isClosed(){return socket->isClosed();}
	string receive();
	void send(const string &s, FrameType ft=TEXT_FRAME);
	bool wait(const int msec=50);
};

#endif /* WEBSOCKET_H_ */
