/*
 * eUpstreamPumpThread.cpp
 *
 *  Created on: 2013. 10. 30.
 *      Author: kos
 */

#include "eUpstreamSocket.h"
#include "ePreDefine.h"

#include <poll.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

using namespace std;

#ifdef DEBUG_LOG
//#undef LOG
//#define LOG(X,...) { do{}while(0); }
#endif

eUpstreamSocket::eUpstreamSocket()
	: mSockFd(-1)
{
}
//-------------------------------------------------------------------------------

eUpstreamSocket::~eUpstreamSocket()
{
}
//-------------------------------------------------------------------------------

bool eUpstreamSocket::Connect()
{
	mSockFd = socket(PF_INET, SOCK_STREAM, 0);

	mSockAddr.sin_family = AF_INET;
	mSockAddr.sin_port = htons(80);
	mSockAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

	if(connect(mSockFd, (struct sockaddr*)&mSockAddr, sizeof(struct sockaddr_in))) {
		return false;
	}
	return true;
}
//-------------------------------------------------------------------------------

int eUpstreamSocket::Send(std::string aData)
{
#ifdef DEBUG_LOG
		LOG("Send : %s", aData.c_str());
#endif
	return write(mSockFd, aData.c_str(), aData.length());
}
//-------------------------------------------------------------------------------

int eUpstreamSocket::Recv(std::string& aData)
{
	int rc = 0;
	char buffer[4096] = {0};
	struct pollfd pollevt;

	pollevt.fd     = mSockFd;
	pollevt.events = POLLIN | POLLERR;

	while(true) {
		buffer[0] = 0;
		pollevt.revents = 0;

		rc = poll((struct pollfd*)&pollevt, 1, 1000);

		if (pollevt.revents == 0) {
			break;
		} else if (pollevt.revents & POLLIN) {
			rc = read(mSockFd, buffer, 4096);
#ifdef DEBUG_LOG
			LOG("Buffer : %s", buffer);
#endif
			aData += buffer;
		}
	}
	return aData.length();
}
//-------------------------------------------------------------------------------

int eUpstreamSocket::Request(std::string aSendData, std::string& aRecvData)
{
	int rc = Send(aSendData);
	if(rc > 0) {
		std::string recvdata;
		rc = Recv(recvdata);
		if(rc > 0) {
			aRecvData = recvdata;
		}
	}
	return rc;
}
//-------------------------------------------------------------------------------


